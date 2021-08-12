#include "EnOceanSTM550BEventAdapter.h"
#include "esp_task_wdt.h"
#include "mbedtls/aes.h"
#include "EnOceanUtils.h"

namespace EnOcean {


STM550BEventAdapter::~STM550BEventAdapter() {
}

void STM550BEventAdapter::registerHandler(Device& device, const uint8_t nodeId) {
  if (stm550bEventHandlerMap.count(nodeId)) {
    registerHandler(device, stm550bEventHandlerMap[nodeId]);
  } else {
    log_e("NodeId [%d] not found in stm550bEventHandlerMap", nodeId);
  }
}

void STM550BEventAdapter::registerHandler(Device& device, STM550BEventHandler* handler) {
  HandlerRegistration reg;
  reg.address    = device.address;
  reg.handler    = handler;
  handlers.push_back(reg);
}

void STM550BEventAdapter::handlePayload(Device& device, Payload& payload) {
  if (securityKeyValid(device, payload)) {
    STM550BEvent event = mapToSTM550BEvent(device, payload);
    callEventHandlers(event);
  }
}

void STM550BEventAdapter::callEventHandlers(STM550BEvent& event) {
  for (auto const& reg : handlers) {
    if (reg.address == event.device->address) {
      reg.handler->handleEvent(event);
    }
  }
}

bool STM550BEventAdapter::securityKeyValid(Device& device, Payload& payload) {
  unsigned char nonce[13] {0};
  uint8_t a0Flag          = 0x01;
  uint8_t b0Flag          = 0x49;
  uint16_t inputLength    = payload.len + 1; // include the length byte 
  unsigned char a0[16]    {0};
  unsigned char b0[16]    {0};
  unsigned char b1[16]    {0};
  unsigned char b2[16]    {0};

  // construct nonce
  memcpy(nonce, device.address.getNative(), 6);
  memcpy(&nonce[6], &payload.sequenceCounter, sizeof(payload.sequenceCounter));


  // construct a0 input parameter
  a0[0] = a0Flag;
  memcpy(&a0[1], nonce, sizeof(nonce));

  // construct b0 input parameter
  b0[0] = b0Flag;
  memcpy(&b0[1], nonce, sizeof(nonce));

  // construct b1 input parameter
  b1[0] = ((inputLength - 4) >> (8 * 1)) & 0xff; // exclude signature
  b1[1] = ((inputLength - 4) >> (8 * 0)) & 0xff;
  b1[2] = payload.len;
  b1[3] = payload.type;
  memcpy(&b1[4], payload.manufacturerId, sizeof(payload.manufacturerId));
  memcpy(&b1[6], &payload.sequenceCounter, sizeof(payload.sequenceCounter));

  if (inputLength <= 18) {
    memcpy(&b1[10], payload.data.raw, inputLength - 12);  
  } else {
    memcpy(&b1[10], payload.data.raw, 6);
    memcpy(b2, payload.data.raw + 6, inputLength - 18);
  }

  unsigned char x1[16] {0};
  unsigned char x1a[16] {0};
  unsigned char x2[16] {0};
  unsigned char x2a[16] {0};
  unsigned char x3[16] {0};
  unsigned char s0[16] {0};
  unsigned char t0[16] {0};

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);

  mbedtls_aes_setkey_enc(&aes, (const unsigned char*)device.securityKey, sizeof(device.securityKey) * 8);

  // calculate X1 from B0
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, b0, x1);


  // xor X1 B1
  for (int i = 0; i < 16; ++i) {
    x1a[i] = x1[i] ^ b1[i];
  }

  // calculate X2 from X1A
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, x1a, x2);

  // Calculate X2A
  for (int i = 0; i < 16; ++i) {
    x2a[i] = x2[i] ^ b2[i];
  }

  // calculate X3 from X2A
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, x2a, x3);

  // calculate S0 from A0
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, a0, s0);
  mbedtls_aes_free(&aes);

  // xor X3 S0
  for (int i = 0; i < 16; ++i) {
    t0[i] = x3[i] ^ s0[i];
  }

  // printBuffer(nonce, 13, false, "Nonce\t");
  // printBuffer(a0, 16, false, "A0\t");
  // printBuffer(b0, 16, false, "B0\t");
  // printBuffer(b1, 16, false, "B1\t");
  // printBuffer(b2, 16, false, "B2\t");
  // printBuffer(x1, 16, false, "X1\t");
  // printBuffer(x1a, 16, false, "X1a\t");
  // printBuffer(x2, 16, false, "X2\t");
  // printBuffer(x2a, 16, false, "X2a\t");
  // printBuffer(x3, 16, false, "X3\t");
  // printBuffer(s0, 16, false, "S0\t");
  // printBuffer(payload.data.signature, 4, false, "Signature");
  // printBuffer(t0, 4, false, "T0\t");

  if (memcmp(t0, payload.data.signature, 4) == 0) {
    return true;
  } else {
    log_e("Incorrect signature");
    return false;
  }
}

STM550BEvent STM550BEventAdapter::mapToSTM550BEvent(Device& device, Payload& payload) {
  STM550BEvent event;

  event.parameters = parsePayloadParameters(payload.data.raw, payload.len - 11);
  event.device = &device;

  return event;
}

} // namespace EnOcean
