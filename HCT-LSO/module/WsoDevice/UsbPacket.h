#pragma once

#include <cstdint>
#include <cmath>


namespace wso_device
{
	// USB Command Packet
	////////////////////////////////////////////////////////////////////////////////////////
	constexpr unsigned int PACKET_DATA_SIZE_MIN = 1;
	constexpr unsigned int PACKET_DATA_SIZE_MAX = 256;
	constexpr unsigned char PACKET_CRC_CODE = '1';
	constexpr unsigned int PACKET_CRC_SIZE = 4;
	constexpr unsigned char PACKET_ID_START = 1;
	constexpr unsigned char PACKET_ID_END = 255;

#pragma pack(push, 1)

	// Command Message Structure
	////////////////////////////////////////////////////////////////////////////////////
	struct NREAD_REQ {
		std::uint8_t nread;
		std::uint8_t pkt_id;
		std::uint8_t addr[4];
		std::uint8_t rd_size;
		std::uint8_t crc[PACKET_CRC_SIZE];
	};

	struct ReadRequest {
		NREAD_REQ packet;
		std::uint16_t size;
	};

	struct NREAD_RES {
		std::uint8_t type;
		std::uint8_t pkt_id;
		std::uint8_t data[PACKET_DATA_SIZE_MAX + PACKET_CRC_SIZE];
	};

	struct ReadResponse {
		NREAD_RES packet;
		std::uint16_t size;
	};

	struct NWRITE_REQ {
		std::uint8_t nwrite;
		std::uint8_t pkt_id;
		std::uint8_t addr[4];
		std::uint8_t wr_size;
		std::uint8_t data[PACKET_DATA_SIZE_MAX + PACKET_CRC_SIZE];
	};

	struct WriteRequest {
		NWRITE_REQ packet;
		std::uint16_t size;
	};

	struct NWRITE_RES {
		std::uint8_t type;
		std::uint8_t pkt_id;
		std::uint8_t status;
		std::uint8_t crc[PACKET_CRC_SIZE];
	};

	struct WriteResponse {
		NWRITE_RES packet;
		std::uint16_t size;
	};

	struct MSG_CMD_RES {
		std::uint8_t type;
		std::uint8_t pkt_id;
		std::uint8_t status;
		std::uint8_t crc[PACKET_CRC_SIZE];
	};

	struct MSG_CMD_HEAD {
		std::uint8_t type;
		std::uint8_t pkt_id;
		std::uint8_t msg_len;
		std::uint16_t ctrl;
	};

	struct MSG_CMD {
		MSG_CMD_HEAD hdr;

		union {
			struct {
				short s1;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} galv1;

			struct {
				short s1;
				short s2;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} galv2;

			struct {
				std::uint8_t c1;
				int n1;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} motor;

			struct {
				std::uint8_t c1;
				std::uint8_t c2;
				std::uint16_t s1;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} sctrl1;

			struct {
				std::uint8_t c1;
				std::float_t f1;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} sctrl2;

			struct {
				std::uint8_t c1;
				std::uint8_t c2;
				std::uint8_t c3;
				std::uint16_t s1;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} sctrl3;

			struct {
				std::uint8_t crc[PACKET_CRC_SIZE];
			} ctrl0;

			struct {
				std::uint8_t c1;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} ctrl1;

			struct {
				std::uint8_t c1;
				std::uint8_t c2;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} ctrl2;

			struct {
				std::uint8_t c1;
				std::float_t c2;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} ctrl3;

			struct {
				std::uint32_t n1;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} ctrl4;

			struct {
				std::uint16_t s1;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} ctrl5;

			struct {
				std::uint16_t s1;
				std::uint16_t s2;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} ctrl6;

			struct {
				std::uint8_t c1;
				std::uint16_t s1;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} ctrl7;

			struct {
				std::uint8_t c1;
				std::uint8_t c2;
				std::uint16_t s1;
				std::uint16_t s2;
				std::uint8_t c3;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} ctrl8;

			struct {
				std::uint8_t c1;
				std::uint8_t c2;
				std::uint8_t c3;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} ctrl9;

			struct {
				std::uint16_t s1;
				std::uint8_t c1;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} ctrl10;

			struct {
				std::uint32_t n1;
				std::uint32_t n2;
				std::uint32_t n3;
				std::uint32_t n4;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} ctrl11;

			struct {
				std::uint8_t c1;
				std::uint32_t n1;
				std::uint32_t n2;
				std::uint32_t n3;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} ctrl12;

			struct {
				std::uint16_t s1;
				std::uint16_t s2;
				std::uint16_t s3;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} scan1;

			struct {
				std::uint16_t s1;
				std::uint16_t s2;
				std::uint16_t s3;
				std::uint16_t s4;
				std::uint16_t s5;
				std::uint8_t crc[PACKET_CRC_SIZE];
			} scan2;
		};
	};

	struct MsgCommand {
		MSG_CMD packet;
		std::uint16_t size;
	};
#pragma pack(pop)
}