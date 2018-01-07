#include "Application.h"
#include "guilib\GraphicContext.h"
#include "utils\Log.h"
#include "utils\Util.h"
#include "utils\SingleLock.h"
#include "GUISettings.h"
#include "guilib\GUIWindowManager.h"
#include "cores\VideoRenderers\RenderManager.h"
#include "guilib\GUIFontManager.h"
#include "guilib\GUIInfoManager.h"
#include "cores\DVDPlayer\DVDPlayer.h"
#include "guilib\LocalizeStrings.h"
#include "Settings.h"
#include "filesystem\File.h"

// Window includes
#include "guilib\windows\GUIWindowHome.h"
#include "guilib\windows\GUIWindowFullScreen.h"
#include "guilib\windows\GUIWindowVideoFiles.h"
#include "guilib\windows\GUIWindowSettings.h"
#include "guilib\windows\GUIWindowSettingsCategory.h"
#include "guilib\windows\GUIWindowScreensaver.h"

//Dialog includes
#include "guilib\dialogs\GUIDialogButtonMenu.h"

CStdString g_LoadErrorStr;

CApplication::CApplication() 
{
	m_pPlayer = NULL;
	m_bPlaybackStarting = false;
	m_dwSkinTime = 0;
	m_bScreenSave = false;
	m_bInitializing = true;
}

CApplication::~CApplication()
{
}

bool CApplication::Create()
{
	CLog::Log(LOGNOTICE, "-----------------------------------------------------------------------");
	CLog::Log(LOGNOTICE, "Starting XBoxMediaCenter.  Built on %s", __DATE__);
	CLog::Log(LOGNOTICE, "-----------------------------------------------------------------------");

	CLog::Log(LOGNOTICE, "Setup DirectX");

	// Create the Direct3D object
	if (!(m_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
	{
		CLog::Log(LOGFATAL, "Unable to create Direct3D!");
		Sleep(INFINITE); // die
	}

	// Transfer the resolution information to our graphics context
	g_graphicsContext.SetD3DParameters(&m_d3dpp);

	if (m_pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_d3dpp, &m_pd3dDevice) != S_OK)
	{
		CLog::Log(LOGFATAL, "Unable to create D3D Device!");
		Sleep(INFINITE); // die
	}

	g_graphicsContext.SetD3DDevice(m_pd3dDevice);

	//Initialize the XUI stuff
	HRESULT hr;

	// Initialize Xui render library with our D3D device, 
    // and use a Xui-provided texture loader.
	hr = XuiRenderInitShared( m_pd3dDevice, &m_d3dpp, XuiD3DXTextureLoader );
	if( FAILED( hr ) ) 
	{
		CLog::Log(LOGFATAL, "Unable to Initialize Xui render library!");
		FatalErrorHandler(true);
	}

    // Create a Xui device context. The Xui text renderer uses many attributes
    // from this device context (position, color, shaders, etc.).
	hr = XuiRenderCreateDC( &m_hXUIDC );
	if( FAILED( hr ) ) 
	{
		CLog::Log(LOGFATAL, "Unable to create Xui device context!");
		FatalErrorHandler(true);
	}

    // Initialize the Xui runtime library.  Typeface descriptors are registered
    // by the runtime library, and consumed by the render library.
	hr = XuiInit( &m_XUIParams );
	if( FAILED( hr ) ) 
	{
		CLog::Log(LOGFATAL, "Unable to initialize the Xui runtime library!");
		FatalErrorHandler(true);
	}

	g_graphicsContext.SetXUIDevice(m_hXUIDC);

	CLog::Log(LOGNOTICE, "Load settings...");	
	if (!g_settings.Load())
		FatalErrorHandler(true);

	CStdString strLanguagePath;
	strLanguagePath.Format("D:\\language\\%s\\strings.xml", g_guiSettings.GetString("LookAndFeel.Language"));

	CLog::Log(LOGINFO, "load language file:%s", strLanguagePath.c_str());
	if (!g_localizeStrings.Load( strLanguagePath ))
		FatalErrorHandler(true);

	return CXBApplicationEX::Create();
}

// This function does not return!
void CApplication::FatalErrorHandler(bool InitD3D)
{
	// XBMC couldn't start for some reason...
	// g_LoadErrorStr should contain the reason
	CLog::Log(LOGWARNING, "Emergency recovery console starting...");

	//
	// TODO
	//
}

bool CApplication::Initialize()
{
	g_windowManager.Add(new CGUIWindowHome); // window id = 0

	CLog::Log(LOGNOTICE, "load default skin:[%s]", g_guiSettings.GetString("LookAndFeel.Skin").c_str());
	LoadSkin(g_guiSettings.GetString("LookAndFeel.Skin"));

	g_windowManager.Add(new CGUIWindowFullScreen);
	g_windowManager.Add(new CGUIWindowVideoFiles);
	g_windowManager.Add(new CGUIWindowSettimgs);
	g_windowManager.Add(new CGUIWindowSettingsCategory);
	g_windowManager.Add(new CGUIWindowScreensaver);

	//Dialogs
	g_windowManager.Add(new CGUIDialogButtonMenu); // window id = 111

	g_windowManager.Initialize();

	g_windowManager.ActivateWindow(WINDOW_HOME);
	
	CLog::Log(LOGNOTICE, "Initialize done");

	m_bInitializing = false;

	// Reset our screensaver (starts timers etc.)
	ResetScreenSaver();

	return true;
}

void CApplication::DelayLoadSkin()
{
	m_dwSkinTime = GetTickCount() + 2000;
	return ;
}

void CApplication::CancelDelayLoadSkin()
{
	m_dwSkinTime = 0;
}

void CApplication::LoadSkin(const CStdString& strSkin)
{
	m_dwSkinTime = 0;

	CStdString strHomePath;
	CStdString strSkinPath = "D:\\skins\\";
	strSkinPath += strSkin;

	CLog::Log(LOGINFO, "Load skin from:%s", strSkinPath.c_str());

	if( IsPlaying() )
	{
		CLog::Log(LOGINFO, "Stop playing...");
		m_pPlayer->CloseFile();
		delete m_pPlayer;
		m_pPlayer = NULL;
	}

	CLog::Log(LOGINFO, "Delete old skin...");
	UnloadSkin();	

	g_graphicsContext.SetMediaDir(strSkinPath);

	CLog::Log(LOGINFO, "Load fonts for skin...");

	CStdString strFontPath = strSkinPath += "\\Fonts.xml";
	g_fontManager.LoadFonts(strFontPath);

	CLog::Log(LOGINFO, "Initialize new skin...");
	g_windowManager.AddMsgTarget(this);
	g_windowManager.Initialize();
	CLog::Log(LOGINFO, "Skin loaded...");
}

void CApplication::ReloadSkin()
{
	CGUIMessage msg(GUI_MSG_LOAD_SKIN, -1, g_windowManager.GetActiveWindow());
	g_windowManager.SendMessage(msg);
	
	// Reload the skin, restoring the previously focused control.  We need this as
	// the window unload will reset all control states.
	CGUIWindow* pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
	int iCtrlID = pWindow->GetFocusedControlID();
	g_application.LoadSkin(g_guiSettings.GetString("LookAndFeel.Skin"));
	pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());

	g_windowManager.ActivateWindow(/*pWindow->GetID()*/WINDOW_SETTINGS_APPEARANCE);
/*	if (pWindow && pWindow->HasSaveLastControl())
	{
		CGUIMessage msg3(GUI_MSG_SETFOCUS, g_windowManager.GetActiveWindow(), iCtrlID, 0); //TODO
		pWindow->OnMessage(msg3);
	}*/
}

void CApplication::UnloadSkin()
{
	g_windowManager.DeInitialize();
	g_TextureManager.Cleanup();
	g_fontManager.Clear();
}

void CApplication::Process()
{
	// Check if we need to load a new skin
	if(m_dwSkinTime && GetTickCount() >= m_dwSkinTime)
	{
		ReloadSkin();
	}

	// Check if we need to activate the screensaver (if enabled)
//	if(g_guiSettings.GetString("ScreenSaver.Mode") != "None") //TODO
		CheckScreenSaver();

	// Process input actions
	ProcessGamepad();

	// Dispatch the messages generated by python or other threads to the current window
	g_windowManager.DispatchThreadMessages();
}

bool CApplication::ProcessGamepad()
{
	//TESTING START

//	ResetScreenSaver(); // TODO Move to OnKey()

	if( m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_X )
	{
		CAction action;
		action.wID = ACTION_SHOW_GUI;
			
		if(!g_windowManager.OnAction(action))
			SwitchToFullScreen();
	}

	if( m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_DPAD_UP )
	{
		CAction action;
		action.wID = ACTION_MOVE_UP;
		g_windowManager.OnAction(action);
	}	
	
	if( m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_DPAD_DOWN )
	{
		CAction action;
		action.wID = ACTION_MOVE_DOWN;
		g_windowManager.OnAction(action);
	}

	if( m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_DPAD_LEFT )
	{
		CAction action;
		action.wID = ACTION_MOVE_LEFT;
		g_windowManager.OnAction(action);
	}	
	
	if( m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_DPAD_RIGHT )
	{
		CAction action;
		action.wID = ACTION_MOVE_RIGHT;
		g_windowManager.OnAction(action);
	}

	if( m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_B )
	{
		if(m_pPlayer)
			m_pPlayer->CloseFile();
	}

	if( m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_START )
	{
		g_infoManager.ToggleShowCodec();
	}

	if( m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_BACK )
	{
		g_windowManager.PreviousWindow();
	}

	if( m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_A )
	{
		CAction action;
		action.wID = ACTION_SELECT_ITEM;
		g_windowManager.OnAction(action);
	}

	//TESTING END

	return true;
}

void CApplication::FrameMove()
{
}

bool CApplication::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_PLAYBACK_STOPPED:
		case GUI_MSG_PLAYBACK_ENDED:
		{
			if (/*message.GetMessage() == GUI_MSG_PLAYBACK_ENDED*/0) //MARTY FIXME WIP - Always delete the player on close atm
			{
				// sending true to PlayNext() effectively passes bRestart to PlayFile()
				// which is not generally what we want (except for stacks, which are
				// handled above)
//				g_playlistPlayer.PlayNext();
			}
			else
			{
				delete m_pPlayer;
				m_pPlayer = NULL;
			}

			if (!IsPlayingVideo() && g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
			{
				g_windowManager.PreviousWindow();
			}
			return true;
		}
		break;

		case GUI_MSG_EXECUTE:
		{
			// user has asked for something to be executed
			if (CUtil::IsBuiltIn(message.GetStringParam()))
				CUtil::ExecBuiltIn(message.GetStringParam());
			else
				return false;
		
			return true;
		}
	}
	return false;
}

void CApplication::Render()
{
	if(!m_pd3dDevice)
		return;

	// Don't do anything that would require graphiccontext to be locked before here in fullscreen.
	// that stuff should go into renderfullscreen instead as that is called from the rendering thread

	// Don't show GUI when playing full screen video
	if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
	{
		if ( g_graphicsContext.IsFullScreenVideo() )
		{
			if (m_pPlayer)
			{
				if (m_pPlayer->IsPaused())
				{
					CSingleLock lock(g_graphicsContext);
//					extern void xbox_video_render_update(bool);
			        g_renderManager.RenderUpdate(true);
//					g_windowManager.UpdateModelessVisibility();
					RenderFullScreen();
					g_windowManager.Render();
//					m_pd3dDevice->BlockUntilVerticalBlank();
					m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
					return;
				}
			}
			Sleep(10);
			return;
		}
	}

	// Enable/Disable video overlay window
	if (IsPlayingVideo() && g_windowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO/* && !m_bScreenSave*/)
	{
		g_graphicsContext.EnablePreviewWindow(true);
	}
	else
	{
		g_graphicsContext.EnablePreviewWindow(false);
	}

	g_graphicsContext.Lock();
	m_pd3dDevice->BeginScene();  
	g_graphicsContext.Unlock();

	// Update our FPS
	g_infoManager.UpdateFPS();

	// Draw GUI

	// Render current windows
	g_windowManager.Render();

	// Now render any dialogs
	g_windowManager.RenderDialogs();

	g_graphicsContext.Lock();
	m_pd3dDevice->EndScene();

	// Present the backbuffer contents to the display
	m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
	g_graphicsContext.Unlock();
}

bool CApplication::NeedRenderFullScreen()
{
	if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
	{
//		if (g_windowManager.HasDialogOnScreen()) return true;
 
		CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)g_windowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
		if (!pFSWin)
			 return false;
		return pFSWin->NeedRenderFullScreen();
	}
	return false;
}

void CApplication::RenderFullScreen()
{
	if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
	{
//		m_guiVideoOverlay.Close(true);
//		m_guiMusicOverlay.Close(true);

		CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)g_windowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
		if (!pFSWin)
			return ;
	
		pFSWin->RenderFullScreen();
	}
}

// SwitchToFullScreen() returns true if a switch is made, else returns false
bool CApplication::SwitchToFullScreen()
{
	// See if we're playing a video, and are in GUI mode
	if ( IsPlayingVideo() && g_windowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
	{
		// then switch to fullscreen mode
		g_windowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
//		g_TextureManager.Flush();
		return true;
	}

	return false;
}

void CApplication::StopPlaying()
{
	int iWin = g_windowManager.GetActiveWindow();
	if ( IsPlaying() )
	{
		 if (m_pPlayer)
			m_pPlayer->CloseFile();

		// turn off visualization window when stopping
		if (iWin == WINDOW_FULLSCREEN_VIDEO)
			g_windowManager.PreviousWindow();
	}
}

void CApplication::OnPlayBackEnded()
{
	if(m_bPlaybackStarting)
		return;

	CLog::Log(LOGDEBUG, "%s - Playback has finished", __FUNCTION__);

	CGUIMessage msg(GUI_MSG_PLAYBACK_ENDED, 0, 0);
	g_windowManager.SendThreadMessage(msg);
}

void CApplication::OnPlayBackStarted()
{
	if(m_bPlaybackStarting)
		return;
}

void CApplication::OnQueueNextItem()
{
}

void CApplication::OnPlayBackStopped()
{
	if(m_bPlaybackStarting)
		return;

	CLog::Log(LOGDEBUG, "%s - Playback was stopped", __FUNCTION__);

	CGUIMessage msg( GUI_MSG_PLAYBACK_STOPPED, 0, 0 );
	g_windowManager.SendThreadMessage(msg);
}

bool CApplication::PlayFile(const string strFile)
{
	// tell system we are starting a file
	m_bPlaybackStarting = true;

	// We should restart the player
	if (m_pPlayer)
	{
		delete m_pPlayer;
		m_pPlayer = NULL;
	}

	if (!m_pPlayer)
	{
		//m_eCurrentPlayer = eNewCore;
		m_pPlayer = (CDVDPlayer*)new CDVDPlayer(*this);//CPlayerCoreFactory::CreatePlayer(eNewCore, *this); //TODO Create a factory
	}

	bool bResult;
	if (m_pPlayer)
	{
		// don't hold graphicscontext here since player
		// may wait on another thread, that requires gfx
//		CSingleExit ex(g_graphicsContext);
		bResult = m_pPlayer->OpenFile(/*item, options*/strFile.c_str()); //FIXME WIP
	}
	else
	{
		CLog::Log(LOGERROR, "Error creating player for item %s (File doesn't exist?)", /*item.GetPath()*/strFile.c_str());
		bResult = false;
	}

	if(bResult)
	{
//		if (m_iPlaySpeed != 1) //TODO
//		{
//			int iSpeed = m_iPlaySpeed;
//			m_iPlaySpeed = 1;
//			SetPlaySpeed(iSpeed);
//		}

		if( IsPlayingAudio() ) //TODO
		{
//			if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
//				g_windowManager.ActivateWindow(WINDOW_VISUALISATION);
		}

		if( IsPlayingVideo() ) //TODO
		{
//			if (g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION)
//				g_windowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);

			// if player didn't manage to switch to fullscreen by itself do it here
			if(g_renderManager.IsStarted()
				&& g_windowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO )
			SwitchToFullScreen();
		}
	}

	m_bPlaybackStarting = false;
	if(bResult)
	{
		// We must have started, otherwise player might send this later
		if(IsPlaying())
			OnPlayBackStarted();
		else
			OnPlayBackEnded();
	}

	return bResult;
}

bool CApplication::IsPlaying() const
{
	if (!m_pPlayer)
		return false;
	if (!m_pPlayer->IsPlaying())
		return false;
	return true;
}

bool CApplication::IsPaused() const
{
	if (!m_pPlayer)
		return false;
	if (!m_pPlayer->IsPlaying())
		return false;
	return m_pPlayer->IsPaused();
}

bool CApplication::IsPlayingAudio() const
{
	if (!m_pPlayer)
		return false;
	if (!m_pPlayer->IsPlaying())
		return false;
	if (m_pPlayer->HasVideo())
		return false;
	if (m_pPlayer->HasAudio())
		return true;
	
	return false;
}

bool CApplication::IsPlayingVideo() const
{
	if (!m_pPlayer)
		return false;
	if (!m_pPlayer->IsPlaying())
		return false;
	if (m_pPlayer->HasVideo())
		return true;

	return false;
}

void CApplication::ResetScreenSaver()
{
	// Reset our timers
//	m_shutdownTimer.StartZero(); //TODO

	// screen saver timer is reset only if we're not already in screensaver mode
	if (!m_bScreenSave)
		m_screenSaverTimer.StartZero();
}

bool CApplication::ResetScreenSaverWindow()
{
	// If Screen saver is active
	if (m_bScreenSave)
	{
		// Disable screensaver
		m_bScreenSave = false;
		m_screenSaverTimer.StartZero();

		if (m_screenSaverMode != "None")
		{
			// we're in screensaver window
			if (g_windowManager.GetActiveWindow() == WINDOW_SCREENSAVER)
				g_windowManager.PreviousWindow();  // show the previous window
		}

		return true;
	}
	else
		return false;
}

void CApplication::CheckScreenSaver()
{
	// If the screen saver window is active, then clearly we are already active
	if (g_windowManager.IsWindowActive(WINDOW_SCREENSAVER))
	{
		m_bScreenSave = true;
		return;
	}

	bool resetTimer = false;
	if (IsPlayingVideo() && !m_pPlayer->IsPaused()) // Are we playing video and it is not paused?
		resetTimer = true;

//	if (IsPlayingAudio() && g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION) // Are we playing some music in fullscreen vis?  //TODO
//		resetTimer = true;

	if (resetTimer)
	{
		m_screenSaverTimer.StartZero();
		return;
	}

	if (m_bScreenSave) // Already running the screensaver
		return;

	if ( m_screenSaverTimer.GetElapsedSeconds() > 10/*g_guiSettings.GetInt("screensaver.time")*/ * 60 ) //TODO - 10min for now
		ActivateScreenSaver();
}

void CApplication::ActivateScreenSaver()
{
	m_bScreenSave = true;

	// Get Screensaver Mode
	m_screenSaverMode = "";//g_guiSettings.GetString("screensaver.mode"); //TODO

	if (m_screenSaverMode != "None")
	{
		g_windowManager.ActivateWindow(WINDOW_SCREENSAVER);
		return ;
	}
}

void CApplication::Stop()
{
    // Update the settings information (volume, uptime etc. need saving)
    if (XFILE::CFile::Exists("D:\\settings.xml"))
    {
		CLog::Log(LOGNOTICE, "Saving settings");
		g_settings.Save();
    }
    else
		CLog::Log(LOGNOTICE, "Settings not saved (settings.xml is not present)");

    m_bStop = true;
    CLog::Log(LOGNOTICE, "Stop all");

    if (m_pPlayer)
    {
		CLog::Log(LOGNOTICE, "Stopping DVDPlayer");
		m_pPlayer->CloseFile();
		delete m_pPlayer;
		m_pPlayer = NULL;
    }

    CLog::Log(LOGNOTICE, "Unload skin");
    UnloadSkin();

	//Windows
	g_windowManager.Delete(WINDOW_HOME);
	g_windowManager.Delete(WINDOW_FULLSCREEN_VIDEO);
	g_windowManager.Delete(WINDOW_VIDEOS);
	g_windowManager.Delete(WINDOW_SETTINGS);
	g_windowManager.Delete(WINDOW_SETTINGS_MYPICTURES); // All the settings categories
	g_windowManager.Delete(WINDOW_SCREENSAVER);

	//Dialogs
	g_windowManager.Delete(WINDOW_DIALOG_BUTTON_MENU);

	CLog::Log(LOGNOTICE, "Destroy");
    Destroy();

	g_guiSettings.Clear();
    g_localizeStrings.Clear();

    CLog::Log(LOGNOTICE, "Stopped");
}