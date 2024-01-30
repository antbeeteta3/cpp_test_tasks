#pragma once
#ifndef __TESTWIDGET2_H__
#define __TESTWIDGET2_H__

#include <PlayrixEngine.h>
#include "sea.h"

///
/// Виджет - основной визуальный элемент на экране.
/// Он отрисовывает себя, а также может содержать другие виджеты.
///
class TestWidget2 : public GUI::Widget
{
public:
	TestWidget2(const std::string& name, rapidxml::xml_node<>* elem);

	void Draw() override;
	void Update(float dt) override;
	
	void AcceptMessage(const Message& message) override;
	
	bool MouseDown(const IPoint& mouse_pos) override;
    void MouseMove(const IPoint& mouse_pos) override {}
    void MouseUp(const IPoint& mouse_pos) override {}

	void KeyPressed(int keyCode) override;
    void CharPressed(int unicodeChar) override {}

private:
	void Init();

private:	
	float _timer;
	
    Sea sea;
	
    float dw_=0, dh_=0; // sizes of a cell

	void DrawState();
    bool show_state = false;

	void DrawMap();
    
    // ship drawing
    void DrawShip();
    Render::Texture* shipTex;
    PathPointCollection ship_path;
    PathPointCollection::iterator path_iter1, path_iter0;
    bool no_path = true;
    bool ship_is_turned = false;
    float course_angle = 0.0f;
    void SetCourseAngle();
    void InitShipPath();
    void UpdateShipPoints();
    void GetShipPosition(float& x, float& y, float& angle);
};

#endif // __TESTAPPDELEGATE2_H__
