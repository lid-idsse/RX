#pragma once
//=====================================================================//
/*!	@file
	@brief	ボタン表示と制御 @n
			・領域内で、「押した」、「離した」がある場合に「選択」と認識する。@n
			・選択時関数を使わない場合、select_id を監視する事で、状態の変化を認識できる。@n
			・選択時関数にはラムダ式を使える。
    @author 平松邦仁 (hira@rvf-rc45.net)
	@copyright	Copyright (C) 2019 Kunihito Hiramatsu @n
				Released under the MIT license @n
				https://github.com/hirakuni45/RX/blob/master/LICENSE
*/
//=====================================================================//
#include <functional>
#include "graphics/widget.hpp"

namespace gui {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief	ボタン・クラス
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	struct button : public widget {

		typedef button value_type;

		typedef std::function<void(uint32_t)> SELECT_FUNC_TYPE;

		static const int16_t round_radius = 6;
		static const int16_t frame_width  = 3;
		static const int16_t box_size     = 30;		///< サイズが省略された場合の標準的なサイズ
		static const int16_t edge_to_title = 4;

	private:

		SELECT_FUNC_TYPE	select_func_;
		uint32_t			select_id_;

	public:
		//-----------------------------------------------------------------//
		/*!
			@brief	コンストラクター
			@param[in]	loc		ロケーション
			@param[in]	str		ボタン文字列
		*/
		//-----------------------------------------------------------------//
		button(const vtx::srect& loc = vtx::srect(0), const char* str = "") noexcept :
			widget(loc, str), select_func_(), select_id_(0)
		{
			if(get_location().size.x <= 0) {
				auto tlen = 0;
				if(str != nullptr) {
					tlen = strlen(str) * 8;
				}
				at_location().size.x = (frame_width + edge_to_title) * 2 + tlen;
			}
			if(get_location().size.y <= 0) {
				at_location().size.y = box_size;
			}
			insert_widget(this);
		}


		button(const button& th) = delete;
		button& operator = (const button& th) = delete;


		//-----------------------------------------------------------------//
		/*!
			@brief	デストラクタ
		*/
		//-----------------------------------------------------------------//
		virtual ~button() { remove_widget(this); }


		//-----------------------------------------------------------------//
		/*!
			@brief	型整数を取得
			@return 型整数
		*/
		//-----------------------------------------------------------------//
		const char* get_name() const override { return "Button"; }


		//-----------------------------------------------------------------//
		/*!
			@brief	ID を取得
			@return ID
		*/
		//-----------------------------------------------------------------//
		ID get_id() const override { return ID::BUTTON; }


		//-----------------------------------------------------------------//
		/*!
			@brief	初期化
		*/
		//-----------------------------------------------------------------//
		void init() override { }


		//-----------------------------------------------------------------//
		/*!
			@brief	選択推移
			@param[in]	inva	無効状態にする場合「true」
		*/
		//-----------------------------------------------------------------//
		void exec_select(bool inva) override
		{
			++select_id_;
			if(select_func_) {
				select_func_(select_id_);
			}
		}


		//-----------------------------------------------------------------//
		/*!
			@brief	セレクト ID の取得
			@return	セレクト ID
		*/
		//-----------------------------------------------------------------//
		uint32_t get_select_id() const noexcept { return select_id_; }


		//-----------------------------------------------------------------//
		/*!
			@brief	セレクト関数への参照
			@return	セレクト関数
		*/
		//-----------------------------------------------------------------//
		SELECT_FUNC_TYPE& at_select_func() noexcept { return select_func_; }


		//-----------------------------------------------------------------//
		/*!
			@brief	描画
			@param[in]	rdr		描画インスタンス
		*/
		//-----------------------------------------------------------------//
		template<class RDR>
		void draw(RDR& rdr) noexcept
		{
			auto r = get_location();
			rdr.set_fore_color(graphics::def_color::White);
			rdr.round_box(r, round_radius);
			if(get_touch_state().level_) {
				rdr.set_fore_color(graphics::def_color::Silver);
			} else {
				rdr.set_fore_color(graphics::def_color::Darkgray);
			}
			r.org  += frame_width;
			r.size -= frame_width * 2;
			rdr.round_box(r, round_radius - frame_width);

			rdr.set_fore_color(graphics::def_color::White);
			auto sz = rdr.at_font().get_text_size(get_title());
			rdr.draw_text(r.org + (r.size - sz) / 2, get_title());
		}
	};
}
