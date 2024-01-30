#pragma once
#ifndef __TESTAPPDELEGATE2_H__
#define __TESTAPPDELEGATE2_H__

#include <PlayrixEngine.h>

class TestAppDelegate2 : public Core::EngineAppDelegate {
public:
	TestAppDelegate2();

	virtual void GameContentSize(int deviceWidth, int deviceHeight, int &width, int &height) override;
	virtual void ScreenMode(DeviceMode &mode) override;

	virtual void RegisterTypes() override;

	virtual void LoadResources() override;
	virtual void OnResourceLoaded() override;

	virtual void OnPostDraw() override;
};

#endif // __TESTAPPDELEGATE2_H__
