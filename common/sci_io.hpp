#pragma once
//=====================================================================//
/*!	@file
	@brief	RX グループ・SCI I/O 制御 @n
			Copyright 2013, 2016 Kunihito Hiramatsu
	@author	平松邦仁 (hira@rvf-rc45.net)
*/
//=====================================================================//
#include "common/renesas.hpp"
#include "vect.h"

/// F_PCKB はボーレートパラメーター計算で必要で、設定が無いとエラーにします。
#ifndef F_PCKB
#  error "sci_io.hpp requires F_PCKB to be defined"
#endif

namespace device {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief  SCI I/O 制御クラス
		@param[in]	SCI	SCI 定義クラス
		@param[in]	RECV_BUFF	受信バッファクラス
		@param[in]	SEND_BUFF	送信バッファクラス
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	template <class SCI, class RECV_BUFF, class SEND_BUFF>
	class sci_io {

		static RECV_BUFF recv_;
		static SEND_BUFF send_;

		uint8_t	level_;
		bool	crlf_;

		// ※必要なら、実装する
		void sleep_() { asm("nop"); }

		static INTERRUPT_FUNC void recv_task_()
		{
			bool err = false;
			if(SCI::SSR.ORER()) {	///< 受信オーバランエラー状態確認
				SCI::SSR = 0x00;	///< 受信オーバランエラークリア
				err = true;
			}
			///< フレーミングエラー/パリティエラー状態確認
			if(SCI::SSR() & (SCI::SSR.FER.b() | SCI::SSR.PER.b())) {
				err = true;
			}
			if(!err) recv_.put(SCI::RDR());
		}

		static INTERRUPT_FUNC void send_task_()
		{
			if(send_.length()) {
				SCI::TDR = send_.get();
			} else {
				SCI::SCR.TIE = 0;
			}
		}

		void set_vector_(ICU::VECTOR rx_vec, ICU::VECTOR tx_vec) {
			if(level_) {
				set_interrupt_task(recv_task_, static_cast<uint32_t>(rx_vec));
				set_interrupt_task(send_task_, static_cast<uint32_t>(tx_vec));
			} else {
				set_interrupt_task(nullptr, static_cast<uint32_t>(rx_vec));
				set_interrupt_task(nullptr, static_cast<uint32_t>(tx_vec));
			}
		}

		bool set_intr_() {
			SCI::SCR = 0x00;			// TE, RE disable.

			set_vector_(SCI::get_rx_vec(), SCI::get_tx_vec());

			icu_mgr::set_level(SCI::get_peripheral(), level_);

			return true;
		}

		void send_restart_() {
			if(!SCI::SCR.TIE() && send_.length() > 0) {
				while(SCI::SSR.TEND() == 0) sleep_();
				char ch = send_.get();
				SCI::TDR = ch;
				SCI::SCR.TIE = 1;
			}
		}

	public:
		//-----------------------------------------------------------------//
		/*!
			@brief  コンストラクター
		*/
		//-----------------------------------------------------------------//
		sci_io() : level_(0), crlf_(true) { }


		//-----------------------------------------------------------------//
		/*!
			@brief  ボーレートを設定して、SCI を有効にする
			@param[in]	baud	ボーレート
			@param[in]	level	割り込みレベル（０の場合ポーリング）
			@return エラーなら「false」
		*/
		//-----------------------------------------------------------------//
		bool start(uint32_t baud, uint8_t level = 0) {

			level_ = level;

			port_map::turn(SCI::get_peripheral());

			uint32_t brr = F_PCKB / baud / 16;
			uint8_t cks = 0;
			while(brr > 512) {
				brr >>= 2;
				++cks;
			}
			if(cks > 3) return false;
			bool abcs = true;
			if(brr > 256) { brr /= 2; abcs = false; }

			power_cfg::turn(SCI::get_peripheral());

			if(!set_intr_()) {
				return false;
			}

			// 8 bits, 1 stop bit, no-parrity
			SCI::SMR = cks;
			SCI::SEMR.ABCS = abcs;
			if(brr) --brr;
			SCI::BRR = static_cast<uint8_t>(brr);

			if(level) {
				SCI::SCR = SCI::SCR.RIE.b() | SCI::SCR.TE.b() | SCI::SCR.RE.b();
			} else {
				SCI::SCR = SCI::SCR.TE.b() | SCI::SCR.RE.b();
			}

			return true;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  通信速度を設定して、SPI を有効にする
			@param[in]	master	マスターモードの場合「true」
			@param[in]	bps	ビットレート
			@param[in]	level	割り込みレベル（０の場合ポーリング）
			@return エラーなら「false」
		*/
		//-----------------------------------------------------------------//
		bool start_spi(bool master, uint32_t bps, uint8_t level = 0)
		{
			crlf_ = false;
			level_ = level;

			uint32_t brr = F_PCKB / bps / 4;
			uint8_t cks = 0;
			while(brr > 256) {
				brr >>= 2;
				++cks;
			}
			if(cks > 3 || brr > 256) return false;

			power_cfg::turn(SCI::get_peripheral());

			if(!set_intr_()) {
				return false;
			}

			// LSB(0), MSB(1) first
			SCI::SCMR.SDIR = 1;

			SCI::SIMR1.IICM = 0;
			SCI::SMR = cks | SCI::SMR.CM.b();
			SCI::SPMR.SSE = 0;		///< SS 端子制御しない「０」

			if(master) {
				SCI::SPMR.MSS = 0;
			} else {
				SCI::SPMR.MSS = 1;
			}

			// クロックタイミング種別選択
			SCI::SPMR.CKPOL = 0;
			SCI::SPMR.CKPH  = 0;

			if(brr) --brr;
			SCI::BRR = static_cast<uint8_t>(brr);

			uint8_t scr = 0;
			if(master) {
				scr = SCI::SCR.CKE.b(0b01);
			} else {
				scr = SCI::SCR.CKE.b(0b10);
			}

			if(level_) {
				SCI::SCR = SCI::SCR.RIE.b() | SCI::SCR.TE.b() | SCI::SCR.RE.b() | scr;
			} else {
				SCI::SCR = SCI::SCR.TE.b() | SCI::SCR.RE.b() | scr;
			}

			return true;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief	CRLF 自動送出
			@param[in]	f	「false」なら無効
		 */
		//-----------------------------------------------------------------//
		void auto_crlf(bool f = true) { crlf_ = f; }


		//-----------------------------------------------------------------//
		/*!
			@brief	SCI 出力バッファのサイズを返す
			@return　バッファのサイズ
		 */
		//-----------------------------------------------------------------//
		uint32_t send_length() const {
			if(level_) {
				return send_.length();
			} else {
				return 0;
			}
		}


		//-----------------------------------------------------------------//
		/*!
			@brief	SCI 文字出力
			@param[in]	ch	文字コード
		 */
		//-----------------------------------------------------------------//
		void putch(char ch) {
			if(crlf_ && ch == '\n') {
				putch('\r');
			}

			if(level_) {
				/// ７／８ を超えてた場合は、バッファが空になるまで待つ。
				if(send_.length() >= (send_.size() * 7 / 8)) {
					send_restart_();
					while(send_.length() != 0) sleep_();
				}
				send_.put(ch);
				send_restart_();
			} else {
				while(SCI::SSR.TEND() == 0) sleep_();
				SCI::TDR = ch;
			}

		}


		//-----------------------------------------------------------------//
		/*!
			@brief	SCI 入力文字数を取得
			@return	入力文字数
		 */
		//-----------------------------------------------------------------//
		uint32_t length() {
			if(level_) {
				return recv_.length();
			} else {
				if(SCI::SSR.ORER()) {	///< 受信オーバランエラー状態確認
					SCI::SSR.ORER = 0;	///< 受信オーバランエラークリア
				}
				auto n = SCI::SSR.RDRF();
				return n;
			}
		}


		//-----------------------------------------------------------------//
		/*!
			@brief	SCI 文字入力
			@return 文字コード
		 */
		//-----------------------------------------------------------------//
		char getch() {
			if(level_) {
				while(recv_.length() == 0) sleep_();
				return recv_.get();
			} else {
				char ch;
				while(length() == 0) sleep_();
				ch = SCI::RDR();	///< 受信データ読み出し
				return ch;
			}
		}


		//-----------------------------------------------------------------//
		/*!
			@brief	uart文字列出力
			@param[in]	s	出力ストリング
		 */
		//-----------------------------------------------------------------//
		void puts(const char* s) {
			char ch;
			while((ch = *s++) != 0) {
				putch(ch);
			}
		}
	};

	template<class SCI, class RECV_BUFF, class SEND_BUFF>
		RECV_BUFF sci_io<SCI, RECV_BUFF, SEND_BUFF>::recv_;
	template<class SCI, class RECV_BUFF, class SEND_BUFF>
		SEND_BUFF sci_io<SCI, RECV_BUFF, SEND_BUFF>::send_;
}
