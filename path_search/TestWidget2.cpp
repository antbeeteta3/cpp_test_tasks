#include "stdafx.h"
#include "TestWidget2.h"

// defines speed of the ship
const float TICK = 0.15f;
const float TICK_d = TICK-0.001;

TestWidget2::TestWidget2(const std::string& name, rapidxml::xml_node<>* elem)
	: Widget(name)
	, _timer(0)	
{
	Init();
}

void TestWidget2::Init()
{
	shipTex = Core::resourceManager.Get<Render::Texture>("Boat");
}

void TestWidget2::Draw()
{	
    // get sizes of a cell
    dw_ = (float)Render::device.Width() / sea.width();
    dh_ = (float)Render::device.Height() / sea.height();

    // background
    Render::device.SetTexturing(false);
    Render::BeginColor(Color(255, 255, 255, 255));
    Render::DrawRect(0, 0, Render::device.Width(), Render::device.Height());
    Render::EndColor();

	DrawMap();
	DrawState();
    DrawShip();
}

void TestWidget2::DrawState()
{
    if (!show_state)
        return;

	Render::device.SetTexturing(false); 
	Render::BeginColor(Color(255, 128, 0, 255));
	Render::DrawRect(Render::device.Width() -200, 0, 200, 40);
	Render::EndColor();
	Render::BindFont("arial");
	
	int dy = Render::getFontHeight();
	int x = Render::device.Width() - 200 + 5;
	int y = 30;

	Render::PrintString(x, y, sea.state());
    if (!sea.limits_ready())
        Render::PrintString(x, y -= dy, "Waiting of start/stop instructions");
    else if (no_path)
	    Render::PrintString(x, y-=dy, "Path not found");
}

void TestWidget2::DrawMap()
{
	if (!sea.loaded())
		return;

	Render::device.SetTexturing(false);
	Render::BeginColor(Color(10, 10, 10, 255));
		
	const static float di1 = 1.5f;	// indents
    const static float di2 = 0.4f;

	for (int i = 1; i < sea.width(); ++i)
		Render::DrawLine(FPoint(dw_*i, 0), FPoint(dw_*i, Render::device.Height()));
	for (int i = 1; i < sea.height(); ++i)
		Render::DrawLine(FPoint(0, dh_*i), FPoint(Render::device.Width(), dh_*i));
	
	Render::EndColor();

	auto draw_obstacle = [this](int row, int col) {
		Render::DrawRect(dw_*col + di1, dh_*row + di1, dw_ - di2, dh_ - di2);
	};
	Render::BeginColor(Color(0, 220, 100, 255));
	sea.walk_obstacles(draw_obstacle);
	Render::EndColor();

    // draw the start, stop points here
	const auto& start = sea.get_start();
	const auto& finish = sea.get_finish();
	Render::BeginColor(Color(0, 100, 220, 255));
	if (!start.empty())
		Render::DrawRect(dw_*start.col + di1, dh_*start.row + di1, dw_ - di2, dh_ - di2);
	if (!finish.empty())
		Render::DrawRect(dw_*finish.col + di1, dh_*finish.row + di1, dw_ - di2, dh_ - di2);
	Render::EndColor();	
}

void TestWidget2::DrawShip()
{
    if (sea.get_start().empty())
        return;
    
    FRect rect(0, dw_, 0, 3*dh_);
    FRect uv(0, 1, 0, 1);
    shipTex->TranslateUV(rect, uv);

    Render::device.PushMatrix();
    
    float x, y, angle;
    GetShipPosition(x, y, angle);

    Render::device.MatrixTranslate(x, y, 0);
    Render::device.MatrixRotate(math::Vector3(0, 0, 1), angle);
    Render::device.MatrixTranslate(-dw_ / 2, -dh_*1.5, 0);
        
    shipTex->Bind();
    Render::DrawQuad(rect, uv);

    Render::device.PopMatrix();
}

void TestWidget2::GetShipPosition(float& x, float& y, float& angle)
{
    if (!no_path) {
        x = path_iter0->col*dw_;
        y = path_iter0->row*dh_;
        angle = course_angle;

        if (ship_is_turned) {
            assert(path_iter1->turn != NONE_TURN);
            if (path_iter1->turn == CLOCKWISE_TURN)
                angle +=  90.f - 90.f*math::clamp(0.0f, TICK_d, _timer) / TICK_d;
            else 
                angle += -90.f + 90.f*math::clamp(0.0f, TICK_d, _timer) / TICK_d;
        }
        else {
            x += dw_*math::clamp(0.0f, TICK_d, _timer) / TICK_d*(path_iter1->col - path_iter0->col);
            y += dh_*math::clamp(0.0f, TICK_d, _timer) / TICK_d*(path_iter1->row - path_iter0->row);
        }
    }
    else {
        angle = 0.0f;
        x = sea.get_start().col*dw_;
        y = sea.get_start().row*dh_;
    }

    x += dw_ / 2;   // middle of cell
    y += dh_ / 2;
}

void TestWidget2::Update(float dt)
{
    bool upd = true; 
    
    _timer += dt;
    while (_timer > TICK) {
		_timer -= TICK;

        if (upd)
            UpdateShipPoints();

        upd = false;
	}
}

void TestWidget2::UpdateShipPoints()
{
    if (no_path)
        return;

    if (ship_is_turned) {   // end of a turn
        ship_is_turned = false;
    }
    else {
        ++path_iter0;
        ++path_iter1;
        if (path_iter1 == ship_path.end()) {
            path_iter0 = ship_path.begin();
            path_iter1 = path_iter0 + 1;
            course_angle = 0.0f;
        }
        SetCourseAngle();
    }
}

void TestWidget2::SetCourseAngle()
{
    if (path_iter1->turn != NONE_TURN) {
        ship_is_turned = true;

        if (path_iter1->turn == CLOCKWISE_TURN)
            course_angle -= 90.f;
        else
            course_angle += 90.f;
    }
    else {
        ship_is_turned = false;
    }
}

bool TestWidget2::MouseDown(const IPoint &mouse_pos)
{
    int col = mouse_pos.x * sea.width() / Render::device.Width();
    int row = mouse_pos.y * sea.height() / Render::device.Height();

    if (Core::mainInput.GetMouseRightButton()) {
        sea.set_finish(row, col);
    }
    else {
        sea.set_start(row, col);
    }

    if (sea.path_ready()) {
        InitShipPath();
    }

    return false;
}

void TestWidget2::InitShipPath()
{
    sea.take_path(ship_path);
    if (ship_path.size() < 2) {
        no_path = true;
        return;
    }

    no_path = false;
    path_iter0 = ship_path.begin();
    path_iter1 = path_iter0 + 1;
    course_angle = 0.0f;
    SetCourseAngle();
}

void TestWidget2::AcceptMessage(const Message& message)
{
	//
	// ¬иджету могут посылатьс€ сообщени€ с параметрами.
	//

	const std::string& publisher = message.getPublisher();
	const std::string& data = message.getData();
}

void TestWidget2::KeyPressed(int keyCode)
{
	//
	// keyCode - виртуальный код клавиши.
	// ¬ качестве значений дл€ проверки нужно использовать константы VK_.
	//

	if (keyCode == VK_SPACE) {
        sea.reload();
        no_path = true;
	}
    if (keyCode == VK_S) {
        show_state ^= true;
    }
}

