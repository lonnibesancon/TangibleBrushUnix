#ifndef VTK_OUTPUT_WINDOW_H
#define VTK_OUTPUT_WINDOW_H

#include "global.h"

#include <vtkOutputWindow.h>
#include <vtkObjectFactory.h>

class VTKOutputWindow : public vtkOutputWindow
{
public:
	static VTKOutputWindow* New();

	virtual void DisplayDebugText(const char* text) { LOGD("%s", text); }
	virtual void DisplayWarningText(const char* text) { LOGW("%s", text); }
	virtual void DisplayErrorText(const char* text) { LOGE("%s", text); }
	virtual void DisplayText(const char* text) { LOGI("%s", text); }
	virtual void DisplayGenericWarningText(const char* text) { LOGW("%s", text); }

	static void install()
	{
		vtkOutputWindow* win = vtkOutputWindow::New();
		vtkOutputWindow::SetInstance(win);
		win->Delete();
	}

protected:
	VTKOutputWindow() {}

private:
	VTKOutputWindow(const VTKOutputWindow&); // not implemented
	void operator=(const VTKOutputWindow&);  // not implemented
};

vtkStandardNewMacro(VTKOutputWindow)

#endif /* VTK_OUTPUT_WINDOW_H */
