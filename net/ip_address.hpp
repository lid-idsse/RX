#pragma once
//=====================================================================//
/*!	@file
	@brief	IP address 定義 @n
			Copyright 2017 Kunihito Hiramatsu
	@author	平松邦仁 (hira@rvf-rc45.net)
*/
//=====================================================================//
#include <cstdint>
#include "common/input.hpp"

namespace net {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief  ip_address クラス
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	class ip_address {

		union address_t {
			uint32_t	dword;
			uint8_t		bytes[4];  // IPv4 address
			address_t(uint32_t v = 0) : dword(v) { }
			address_t(uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3) {
				bytes[0] = v0;
				bytes[1] = v1;
				bytes[2] = v2;
				bytes[3] = v3;
			}
		};

		address_t	address_;

	public:
		//-----------------------------------------------------------------//
		/*!
			@brief  コンストラクター
			@param[in]	dword	32bits IP
		*/
		//-----------------------------------------------------------------//
		ip_address(uint32_t dword = 0) : address_(dword) { }


		//-----------------------------------------------------------------//
		/*!
			@brief  コンストラクター
			@param[in]	first	IP 1ST
			@param[in]	second	IP 2ND
			@param[in]	third	IP 3RD
			@param[in]	fourth	IP 4TH
		*/
		//-----------------------------------------------------------------//
		ip_address(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth) :
			address_(first, second, third, fourth) { }


		//-----------------------------------------------------------------//
		/*!
			@brief  コンストラクター
			@param[in]	address	配列
		*/
		//-----------------------------------------------------------------//
		ip_address(const uint8_t* address) {
			if(address == nullptr) {
				address_ = 0;
				return;
			}
			address_.bytes[0] = address[0];
			address_.bytes[1] = address[1];
			address_.bytes[2] = address[2];
			address_.bytes[3] = address[3];
		}

		bool from_string(const char *address) {
			if(address == nullptr) return false;
			int a, b, c, d;
			auto f = (utils::input("%d.%d.%d.%d", address) % a % b % c % d).status();
			if(f) {
				if(a >= 0 && a <= 255) address_.bytes[0] = a;
				if(a >= 0 && b <= 255) address_.bytes[1] = b;
				if(a >= 0 && c <= 255) address_.bytes[2] = c;
				if(a >= 0 && d <= 255) address_.bytes[3] = d;
			}
			return f;
		}

		operator uint32_t() const { return address_.dword; };

		bool operator==(const ip_address& addr) const {
			return address_.dword == addr.address_.dword;
		}


		bool operator == (const uint8_t* addr) const {
			return memcmp(addr, address_.bytes, sizeof(address_.bytes)) == 0;
		}

		uint8_t operator [] (int index) const { return address_.bytes[index]; };


		uint8_t& operator [] (int index) { return address_.bytes[index]; };


		ip_address& operator = (const uint8_t* address) {
			memcpy(address_.bytes, address, sizeof(address_.bytes));
			return *this;
		}

		ip_address& operator = (uint32_t address) {
			address_.dword = address;
			return *this;
		}
	};
}

