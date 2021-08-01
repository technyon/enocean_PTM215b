#include "EnOceanEMDCBEventAdapter.h"
#include "esp_task_wdt.h"
#include "mbedtls/aes.h"
#include "EnOceanUtils.h"

namespace EnOcean {


EMDCBEventAdapter::~EMDCBEventAdapter() {
}

void EMDCBEventAdapter::registerHandler(Device& device, const uint8_t nodeId) {
  if (emdcbEventHandlerMap.count(nodeId)) {
    registerHandler(device, emdcbEventHandlerMap[nodeId]);
  } else {
    log_e("NodeId [%d] not found in ptm215EventHandlerMap", nodeId);
  }
}

void EMDCBEventAdapter::registerHandler(Device& device, EMDCBEventHandler* handler) {
  HandlerRegistration reg;
  reg.address    = device.address;
  reg.handler    = handler;
  handlers.push_back(reg);
}

void EMDCBEventAdapter::handlePayload(Device& device, Payload& payload) {
  if (securityKeyValid(device, payload)) {
    EMDCBEvent event = mapToEMDCBEvent(device, payload);
    callEventHandlers(event);
  }
}

void EMDCBEventAdapter::callEventHandlers(EMDCBEvent& event) {
  for (auto const& reg : handlers) {
    if (reg.address == event.device->address) {
      reg.handler->handleEvent(event);
    }
  }
}

bool EMDCBEventAdapter::securityKeyValid(Device& device, Payload& payload) {
  unsigned char nonce[13] = {0};
  uint8_t a0Flag          = 0x01;
  uint8_t b0Flag          = 0x49;
  uint16_t inputLength    = payload.len;
  unsigned char a0[16]    = {0};
  unsigned char b0[16]    = {0};
  unsigned char b1[16]    = {0};
  unsigned char b2[16]    = {0};

  // construct nonce
  memcpy(nonce, device.address.getNative(), 6);
  memcpy(&nonce[6], &payload.sequenceCounter, sizeof(payload.sequenceCounter));

  printBuffer(nonce, 13, false, "Nonce\t");

  // construct a0 input parameter
  a0[0] = a0Flag;
  memcpy(&a0[1], nonce, sizeof(nonce));
  printBuffer(a0, 16, false, "A0\t");

  // construct b0 input parameter
  b0[0] = b0Flag;
  memcpy(&b0[1], nonce, sizeof(nonce));
  printBuffer(b0, 16, false, "B0\t");

  // construct b1 input parameter
  b1[0] = ((inputLength -3) >> (8 * 1)) & 0xff; // length without CRC
  b1[1] = ((inputLength -3) >> (8 * 0)) & 0xff;
  b1[2] = payload.len;
  b1[3] = payload.type;
  memcpy(&b1[4], payload.manufacturerId, sizeof(payload.manufacturerId));
  memcpy(&b1[6], &payload.sequenceCounter, sizeof(payload.sequenceCounter));

  if (inputLength < 14) {
    memcpy(&b1[10], payload.data.raw, inputLength - 6);  
  } else {
    memcpy(&b1[10], payload.data.raw, 7);
    memcpy(b2, payload.data.raw + 6, inputLength - 13);
  }
  printBuffer(b1, 16, false, "B1\t");
  printBuffer(b2, 16, false, "B2\t");

  unsigned char x1[16]  = {0};
  unsigned char x1a[16] = {0};
  unsigned char x2[16]  = {0};
  unsigned char x2a[16] = {0};
  unsigned char x3[16] = {0};
  unsigned char s0[16]  = {0};
  unsigned char t0[16]  = {0};

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);

  mbedtls_aes_setkey_enc(&aes, (const unsigned char*)device.securityKey, sizeof(device.securityKey) * 8);

  // calculate X1 from B0
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, b0, x1);
  printBuffer(x1, 16, false, "X1\t");


  // xor X1 B1
  for (int i = 0; i < 16; ++i) {
    x1a[i] = x1[i] ^ b1[i];
  }
  printBuffer(x1a, 16, false, "X1a\t");

  // calculate X2 from X1A
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, x1a, x2);
  printBuffer(x2, 16, false, "X2\t");

  // Calculate X2A
  for (int i = 0; i < 16; ++i) {
    x2a[i] = x2[i] ^ b2[i];
  }

  // calculate X3 from X2A
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, x2a, x3);

  // calculate S0 from A0
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, a0, s0);
  mbedtls_aes_free(&aes);
  printBuffer(s0, 16, false, "S0\t");

  // xor X3 S0
  for (int i = 0; i < 16; ++i) {
    t0[i] = x3[i] ^ s0[i];
  }

  printBuffer(payload.data.signature, 4, false, "Signature");
  printBuffer(t0, 4, false, "T0\t");


  if (memcmp(t0, payload.data.signature, 4) == 0) {
    return true;
  } else {
    log_e("Incorrect signature");
    return false;
  }
}

EMDCBEvent EMDCBEventAdapter::mapToEMDCBEvent(Device& device, Payload& payload) {
  EMDCBEvent event;

  std::vector<PayloadVariable> variables = parsePayload(payload.data.raw, payload.len - 4);
  for (auto const& var : variables) {
    Parameter param;
    param.type = (ParameterType) var.type;
    param.value = var.value;
    event.parameters.push_back(param);
  }
  event.device = &device;

  return event;
}

} // namespace EnOcean
