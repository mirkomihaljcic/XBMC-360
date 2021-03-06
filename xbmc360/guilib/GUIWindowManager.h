#ifndef GUILIB_CGUIWindowManager_H
#define GUILIB_CGUIWindowManager_H

#include "..\utils\Stdafx.h"
#include "..\utils\CriticalSection.h"
#include "IMsgTargetCallback.h"
#include "GUIWindow.h"
#include "GUIMessage.h"

#include <map>
#include <stack>
#include <vector>

class CGUIWindowManager
{
public:
	CGUIWindowManager(void);
	virtual ~CGUIWindowManager(void);

	void Initialize();

	/*! \brief Return whether the window manager is initialized.
	The window manager is initialized on skin load - if the skin isn't yet loaded,
	no windows should be able to be initialized.
	\return true if the window manager is initialized, false otherwise.
	*/
	bool Initialized() const { return m_initialized; };
	void SendThreadMessage(CGUIMessage& message);
	void DispatchThreadMessages();
	bool SendMessage(CGUIMessage& message);
	void Add(CGUIWindow* pWindow);
	void LoadNotOnDemandWindows();
	void UnloadNotOnDemandWindows();
	CGUIWindow* GetWindow(int id) const;
	int GetActiveWindow() const;
	bool HasDialogOnScreen() const;
	bool IsWindowActive(int id);
	void PreviousWindow();
	void Delete(int id);
	void Remove(int id);
	void RouteToWindow(CGUIWindow* pDialog);
	void AddModeless(CGUIWindow* pDialog);
	void RemoveDialog(DWORD dwID);
	void ChangeActiveWindow(int iNewID);
	void ActivateWindow(int iWindowID, bool swappingWindows = false);

	// OnAction() runs through our active dialogs and windows and sends the message
	// off to the callbacks (application, python, playlist player) and to the
	// currently focused window(s).  Returns true only if the message is handled.
	bool OnAction(const CAction &action);

	void Render();
	void RenderDialogs();

	/*! \brief Per-frame updating of the current window and any dialogs
	FrameMove is called every frame to update the current window and any dialogs
	on screen. It should only be called from the application thread.
	*/
	void FrameMove();
	void AddToWindowHistory(int newWindowID);
	void DeInitialize();
	void ClearWindowHistory();
	void AddMsgTarget( IMsgTargetCallback* pMsgTarget );

private:
	void Render_Internal();
	typedef std::map<int, CGUIWindow *> WindowMap;
	WindowMap m_mapWindows;

	std::stack<int> m_windowHistory;

	std::vector <CGUIWindow*> m_activeDialogs;
	std::vector < std::pair<CGUIMessage*,int> > m_vecThreadMessages;
	typedef std::vector<CGUIWindow*>::iterator iDialog;

	CCriticalSection m_critSection;

	bool m_initialized;

	vector <IMsgTargetCallback*> m_vecMsgTargets;
};

extern CGUIWindowManager g_windowManager;

#endif //GUILIB_CGUIWindowManager_H