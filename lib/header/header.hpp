#ifndef HEADER_HPP
#define HEADER_HPP

#include <stdint.h>

class Header {
public:
    // Champs visibles
    uint8_t type;     // 4 bits
    uint8_t flags;    // 4 bits
    uint16_t id;      // 16 bits
    uint16_t ttl;     // 16 bits
    uint16_t target;  // 16 bits
    uint16_t length;  // 16 bits
    uint16_t checksum; // 16 bits

    Header() {}

    Header(uint8_t type_, uint8_t flags_, uint16_t id_, uint16_t ttl_,
           uint16_t target_, uint16_t length_) {
        set(type_, flags_, id_, ttl_, target_, length_);
        checksum = computeChecksum();
    }

    void set(uint8_t type_, uint8_t flags_, uint16_t id_, uint16_t ttl_,
             uint16_t target_, uint16_t length_) {
        type    = type_  & 0x0F;
        flags   = flags_ & 0x0F;
        id      = id_;
        ttl     = ttl_;
        target  = target_;
        length  = length_;
    }

    // Calcule un checksum simple (somme des mots sur 16 bits)
    uint16_t computeChecksum() const {
        uint32_t sum = 0;
        sum += ((type & 0x0F) << 12) | ((flags & 0x0F) << 8) | ((id >> 8) & 0xFF);
        sum += ((id & 0xFF) << 8) | ((ttl >> 8) & 0xFF);
        sum += ((ttl & 0xFF) << 8) | ((target >> 8) & 0xFF);
        sum += ((target & 0xFF) << 8) | ((length >> 8) & 0xFF);
        sum += ((length & 0xFF) << 8);
        return (uint16_t)(sum % 65536);
    }

    // Encode en 12 octets
    void toBytes(uint8_t* buffer) const {
        buffer[0] = (type << 4) | (flags & 0x0F);
        buffer[1] = id >> 8;
        buffer[2] = id & 0xFF;
        buffer[3] = ttl >> 8;
        buffer[4] = ttl & 0xFF;
        buffer[5] = target >> 8;
        buffer[6] = target & 0xFF;
        buffer[7] = length >> 8;
        buffer[8] = length & 0xFF;
        buffer[9] = checksum >> 8;
        buffer[10] = checksum & 0xFF;
        buffer[11] = 0x00; // padding ou champ réservé, à définir plus tard
    }

    // Decode depuis 12 octets
    void fromBytes(const uint8_t* buffer) {
        type    = (buffer[0] >> 4) & 0x0F;
        flags   = buffer[0] & 0x0F;
        id      = (buffer[1] << 8) | buffer[2];
        ttl     = (buffer[3] << 8) | buffer[4];
        target  = (buffer[5] << 8) | buffer[6];
        length  = (buffer[7] << 8) | buffer[8];
        checksum = (buffer[9] << 8) | buffer[10];
    }

    // Vérifie l'intégrité du header
    bool isValid() const {
        return checksum == computeChecksum();
    }
};

#endif // HEADER_HPP
