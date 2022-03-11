# include <Siv3D.hpp> // OpenSiv3D v0.6.3

/*
　Bluetooth Classicにより遠隔でLEDを点滅させるためのプログラム
　ボーレートは115200で固定

　Bluetooth Classicだとどうも発信と着信２つのポートを用いて通信している
　＊＊＊着信側のポートでwirteしてしまうとフリーズするので注意＊＊＊
　なんとかしようとしたけど現在（2022/03/09)のOpenSiv3Dの機能だと無理そう
　（そもそもBluetoothの機能はない）
　直接WindowsのAPIを叩く必要があるっぽい
　とてもめんどくさすぎるのでそのままにします

　Windows10の設定→デバイス→関連設定のその他のBluetoothオプション→COMポートタブ
　でそのポートが発信か着信か確認できます

　構成
 　・Port選択シーン
 　・LEDチカシーン
 　　・手動モード
 　　・点滅モード
*/

// 各シーンで共通のデータ
struct CommonData
{
public:
	// ボーレートは固定(後で可変にするかも)
	const int32 baud_rate = 9600;

	// Port情報
	SerialPortInfo spi;
};

// エイリアス
using App = SceneManager<String, CommonData>;

// プルダウンリストのクラス(OpenSiv3Dのサンプル集にあったものをほぼそのまま利用している)
class Pulldown
{
public:

	Pulldown() = default;

	Pulldown(const Array<String>& items, const Font& font, const Point& pos = { 0,0 })
		: m_font{ font }
		, m_items{ items }
		, m_rect{ pos, 0, (m_font.height() + m_padding.y * 2) }
	{
		for (const auto& item : m_items)
		{
			m_rect.w = Max(m_rect.w, static_cast<int32>(m_font(item).region().w));
		}

		m_rect.w += (m_padding.x * 2 + m_downButtonSize);
	}

	bool isEmpty() const
	{
		return m_items.empty();
	}

	void update()
	{
		if (isEmpty())
		{
			return;
		}

		if (m_rect.leftClicked())
		{
			m_isOpen = (not m_isOpen);
		}

		Point pos = m_rect.pos.movedBy(0, m_rect.h);

		if (m_isOpen)
		{
			for (auto i : step(m_items.size()))
			{
				if (const Rect rect{ pos, m_rect.w, m_rect.h };
					rect.leftClicked())
				{
					m_index = i;
					m_isOpen = false;
					break;
				}

				pos.y += m_rect.h;
			}
		}
	}

	void draw() const
	{
		m_rect.draw();

		if (isEmpty())
		{
			return;
		}

		m_rect.drawFrame(1, 0, m_isOpen ? Palette::Orange : Palette::Gray);

		Point pos = m_rect.pos;

		m_font(m_items[m_index]).draw(pos + m_padding, Palette::Black);

		Triangle{ (m_rect.x + m_rect.w - m_downButtonSize / 2.0 - m_padding.x), (m_rect.y + m_rect.h / 2.0),
			(m_downButtonSize * 0.5), 180_deg }.draw(Palette::Black);

		pos.y += m_rect.h;

		if (m_isOpen)
		{
			const Rect backRect{ pos, m_rect.w, (m_rect.h * m_items.size()) };

			backRect.drawShadow({ 1, 1 }, 4, 1).draw();

			for (const auto& item : m_items)
			{
				if (const Rect rect{ pos, m_rect.size };
					rect.mouseOver())
				{
					rect.draw(Palette::Skyblue);
				}

				m_font(item).draw((pos + m_padding), Palette::Black);

				pos.y += m_rect.h;
			}

			backRect.drawFrame(1, 0, Palette::Gray);
		}
	}

	void setPos(const Point& pos)
	{
		m_rect.setPos(pos);
	}

	const Rect& getRect() const
	{
		return m_rect;
	}

	size_t getIndex() const
	{
		return m_index;
	}

	String getItem() const
	{
		if (isEmpty())
		{
			return{};
		}

		return m_items[m_index];
	}

	bool get_open() const
	{
		return m_isOpen;
	}

private:

	Font m_font;

	Array<String> m_items;

	size_t m_index = m_items.size()-1;

	Size m_padding{ 6, 2 };

	Rect m_rect;

	int32 m_downButtonSize = 16;

	bool m_isOpen = false;
};

// スイッチクラス（自作）
class Switch_ratio
{
private:
	// 使うフォント
	Font m_font;

	// Switchがオンのとき表す変数
	String on_item;
	// Switchがオフのとき表す変数
	String off_item;

	// Switchがオンになっているかどうか
	bool switch_on;

	// Switchの描画表現
	RectF m_rect; // 土台
	RectF on_rect; // オンに対応する四角
	RectF off_rect; // オフに対応する四角
	Size m_padding = { 8,4 };

public:
	// 何も引数がないときのコンストラクタ
	Switch_ratio() = default;

	// 引数ありのコンストラクタ
	Switch_ratio(const String& on_item, const String& off_item, const Font& font, const Vec2& pos = { 0,0 })
		:m_font{ font }, on_item{ on_item }, off_item{ off_item }, switch_on{ true }
	{
		m_rect = RectF{ pos, 0, (m_font.height() + m_padding.y * 6) };
		// 土台部分の長方形の幅を決定する
		m_rect.w = Max(static_cast<int32>(m_font(on_item).region().w), static_cast<int32>(m_font(off_item).region().w)) * 2;
		m_rect.w += m_padding.x * 6;

		// オン・オフに対応する長方形を決定する。
		on_rect = RectF{ m_rect.pos + Vec2{m_padding.x, m_padding.y}, (m_rect.w - 4 * m_padding.x) / 2, m_rect.h - m_padding.y * 2 };
		off_rect = RectF{ m_rect.pos + Vec2{m_rect.w / 2 + m_padding.x, m_padding.y}, (m_rect.w - 4 * m_padding.x) / 2, m_rect.h - m_padding.y * 2 };
	}

	// 更新関数
	void update()
	{
		// スイッチが押されたらそれに対応させる
		if (on_rect.leftClicked())
		{
			switch_on = true;
		}
		if (off_rect.leftClicked())
		{
			switch_on = false;
		}
	}

	// 描画関数
	void draw() const
	{
		// 土台の描画
		m_rect.rounded(3).drawShadow(Vec2{ 2,2 }, 8, 1).draw(Palette::Aliceblue);

		// オン・オフ部分の描画
		on_rect.rounded(3).draw(switch_on ? Palette::Lavender : Palette::Lightslategray);
		off_rect.rounded(3).draw(switch_on ? Palette::Lightslategray : Palette::Lavender);

		// マウスが置かれているボタンを囲む
		if (on_rect.mouseOver() && switch_on == false)
		{
			on_rect.rounded(3).drawFrame(0, 2, Palette::Aquamarine);
		}
		if (off_rect.mouseOver() && switch_on == true)
		{
			off_rect.rounded(3).drawFrame(0, 2, Palette::Aquamarine);
		}

		// 文字の描画
		m_font(on_item).drawAt(on_rect.center(), Palette::Black);
		m_font(off_item).drawAt(off_rect.center(), Palette::Black);
	}

	// switch_onを取得する関数
	bool get_switch() const
	{
		return switch_on;
	}

	// swichの中身を取得する関数
	String get_item(bool on) const
	{
		if (on)
		{
			return on_item;
		}
		else
		{
			return off_item;
		}
	}

	// スイッチの位置変更のための関数
	void setPos(const Vec2& pos)
	{
		m_rect.setPos(pos);
	}
};

// Port選択シーン
class Port_Select : public App::Scene
{
private:

	// Port情報の取得
	Array<SerialPortInfo> serial_infos = System::EnumerateSerialPorts();
	// 見つかったPortの名前の一覧
	Array<String> port_names = serial_infos.map([](const SerialPortInfo& serial_info)
	{
		return U"{} ({})"_fmt(serial_info.port, serial_info.description);
	}) << U"none";
	// 見つかったPort数
	size_t ports_num = port_names.size() - 1;

	// プルダウンリスト
	Pulldown pulldown_ports{ port_names, FontAsset(U"text"), {40,200} };

public:

	// コンストラクタ
	Port_Select(const InitData& init)
		:IScene{ init }
	{}

	// 更新関数
	void update() override
	{
		// 更新ボタンでPort情報を更新する
		if (SimpleGUI::Button(U"RELOAD", Vec2{ 50, 270 }, 100, pulldown_ports.get_open()==0))
		{
			serial_infos = System::EnumerateSerialPorts();
			port_names = serial_infos.map([](const SerialPortInfo& serial_info)
			{
				return U"{} ({})"_fmt(serial_info.port, serial_info.description);
			}) << U"none";
		}

		// 選択されているPortの情報をCommonDataに保存しLED_Blinkシーンへ遷移
		if (SimpleGUI::Button(U"OPEN", Vec2{ 160, 270 }, 100, (pulldown_ports.getIndex()!=ports_num)&&pulldown_ports.get_open()==0))
		{
			getData().spi = serial_infos[pulldown_ports.getIndex()];
			changeScene(U"LED_Blink", 0.5s);
		}

		// プルダウンリストの更新
		pulldown_ports.update();
	}

	// 描画関数
	void draw() const override
	{
		// 背景の色
		Scene::SetBackground(Palette::Lightcyan);

		// タイトルの描画
		FontAsset(U"title")(U"LED Blink through Bluetooth Serial").draw(40, 45, Palette::Navy);

		//プルダウンリストの説明
		FontAsset(U"text")(U"Port:").draw(40, 170, Palette::Black);

		// プルダウンリストの描画
		pulldown_ports.draw();
	}

};

// LED点滅シーン
class LED_Blink : public App::Scene
{
private:
	// 用いるポート番号
	const SerialPortInfo spi = getData().spi;
	const int32 baud_rate = getData().baud_rate;

	// シリアル通信用クラス
	Serial serial;

	// 手動モード→1　点滅モード→0　とする
	// モード切り替え用のスイッチ
	Switch_ratio sr = { U"手動",U"点滅",FontAsset(U"text"), Vec2{100, 100} };

	// 点滅モードのときの間隔を保持する値
	double time_delay=100;

public:
	// コンストラクタ
	LED_Blink(const InitData& init)
		:IScene{init}
	{
		// シリアル通信を開始
		try
		{
			if (serial.open(spi.port, baud_rate))
			{}
			else
			{
				throw(U"Error: Port open failed.");
			}
		}
		catch(String e)
		{
			Print << e;
		}
	}

	// 更新関数
	void update() override
	{
		// シリアル通信が可能か
		bool isOpen = serial.isOpen();

		// スイッチのアップデート
		sr.update();

		// 手動モードについて
		// データの送信
		// 消灯ボタン
		if (SimpleGUI::Button(U"OFF", Vec2{ 300, 240 }, 120, isOpen&&sr.get_switch()==1))
		{
			// 1 バイトのデータ (10000000) を書き込む
			try
			{
				if (not serial.writeByte(0b10000000))
				{
					throw(U"ERROR: writeByte 10000000");
				}
			}
			catch(String e)
			{
				Print << e;
			}
		}

		// 点灯ボタン
		if (SimpleGUI::Button(U"ON", Vec2{ 440, 240 }, 120, isOpen&&sr.get_switch()==1))
		{
			// 1 バイトのデータ (10100000) を書き込む
			try
			{
				if (not serial.writeByte(0b10100000))
				{
					throw(U"ERROR: writeByte 10100000");
				}
			}
			catch (String e)
			{
				Print << e;
			}
		}

		// 点滅モードについて
		// スライダーで取った値を2 Byte目として送信する
		SimpleGUI::Slider(U"{} ms"_fmt(static_cast<int32>(time_delay) * 10), time_delay, 0, 255, Vec2{ 300,300 }, 100, 200, sr.get_switch() == 0);
		if (SimpleGUI::Button(U"Apply", Vec2{300, 350}, 120, isOpen&&sr.get_switch()==0))
		{
			// 1 Byte目の送信 1 Byteのデータ(11000000)を書き込む
			// 2 Byte目の送信 time_delayを1 Byteにして書き込む
			try
			{
				if (not serial.writeByte(0b11000000))
				{
					throw(U"ERROR: writeByte 11000000");
				}
				if (not serial.writeByte(static_cast<uint8_t>(time_delay)))
				{
					throw(U"ERROR: writeByte delay time");
				}
			}
			catch (String e)
			{
				Print << e;
			}
		}


		// 終了ボタン
		if (SimpleGUI::Button(U"EXIT", Vec2{ 500, 500 }))
		{
			// 1 バイトのデータ (00000000) を書き込む
			try
			{
				if (not serial.writeByte(0b00000000))
				{
					throw(U"ERROR: writeByte 00000000");
				}
			}
			catch (String e)
			{
				Print << e;
			}

			// ポート選択シーンへ遷移
			changeScene(U"Port_Select", 0.1s);
		}
	}

	// 描画関数
	void draw() const override
	{
		// 背景の色
		Scene::SetBackground(Palette::Lightcyan);

		// スイッチの描画
		sr.draw();
	}
};

void Main()
{
	// タイトル
	Window::SetTitle(U"LED Blink through Bluetooth Serial");

	// シーンマネージャの作成
	App manager;

	// シーンの追加
	manager.add<Port_Select>(U"Port_Select");
	manager.add<LED_Blink>(U"LED_Blink");

	// FontAsset
	FontAsset::Register(U"text", 24, Typeface::Medium);
	FontAsset::Register(U"title", 40, Typeface::Bold);

	while (System::Update())
	{
		if (not manager.update())
		{
			break;
		}
	}
}
