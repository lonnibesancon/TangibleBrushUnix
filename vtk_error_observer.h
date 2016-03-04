#ifndef VTK_ERROR_OBSERVER_H
#define VTK_ERROR_OBSERVER_H

#include "global.h"

#include <vtkCommand.h>

// http://www.cmake.org/Wiki/VTK/Examples/Cxx/Utilities/ObserveError
class VTKErrorObserver : public vtkCommand
{
public:
	VTKErrorObserver()
	 : mHasError(false), mHasWarning(false)
	{}

	static VTKErrorObserver* New()
	{ return new VTKErrorObserver; }

	virtual void Execute(vtkObject* vtkNotUsed(caller),
	                     unsigned long event,
	                     void* calldata)
	{
		switch (event) {
			case vtkCommand::ErrorEvent:
				mErrorMessage = static_cast<char*>(calldata);
				mHasError = true;
				break;
			case vtkCommand::WarningEvent:
				mWarningMessage = static_cast<char*>(calldata);
				mHasWarning = true;
				break;
		}
	}

	bool hasError() const { return mHasError; }
	bool hasWarning() const { return mHasWarning; }

	std::string getErrorMessage() const { return mErrorMessage; }
	std::string getWarningMessage() const { return mWarningMessage; }

	void clear()
	{
		mHasError = false;
		mHasWarning = false;
		mErrorMessage.clear();
		mWarningMessage.clear();
	}

private:
	bool mHasError;
	bool mHasWarning;
	std::string mErrorMessage;
	std::string mWarningMessage;
};

#endif /* VTK_ERROR_OBSERVER_H */
