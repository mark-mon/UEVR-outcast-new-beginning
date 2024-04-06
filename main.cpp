#define  _CRT_SECURE_NO_WARNINGS 1
#define  _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <memory>
#include <ctime>
#include <iostream>
#include <fstream>

#include "uevr/Plugin.hpp"
#include "DialogueManager.hpp"
#include "CinematicManager.hpp"

#define DELAY_VALUE 2000
#define MAX_ELEMENT_LEN 128
#define MAX_PATH_SIZE 512
#define MAX_LINE_SIZE 256

typedef struct _TIMER_STRUCT
{
    bool* XPressed;
    std::time_t* TimeStamp;
}   TIMER_STRUCT;

typedef enum _TP_CAUSE {
    CAUSE_NONE = 0,
    CAUSE_DIALOGUE,
    CAUSE_CINEMATIC,
    CAUSE_RIGHT_STICK,
	CAUSE_RBLB,
    CAUSE_X
} TP_CAUSE;

void DebugPrint(char* Format, ...);
using namespace uevr;

#define PLUGIN_LOG_ONCE(...) \
    static bool _logged_ = false; \
    if (!_logged_) { \
        _logged_ = true; \
        API::get()->log_info(__VA_ARGS__); \
    }


DWORD WINAPI
TimerCallbackThreadProc(
  LPVOID    lpParameter   // thread data
    )
{
    TIMER_STRUCT*   ThisTimer = (TIMER_STRUCT*)lpParameter;
    static bool ThirdPerson = false;
    
    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = VK_F4;
    
    while(1) {
        Sleep(10);
        
        if (*ThisTimer->XPressed == true) {
            ThirdPerson = true;
            input.ki.dwFlags = 0;
            SendInput(1, &input, sizeof(INPUT));
            Sleep(50);
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
        }
        
        while(ThirdPerson == true) {
            Sleep(DELAY_VALUE);
            
            if(ThirdPerson == false) break;
            
            // See if the timestamp is still the same or has been updated.
            if(std::time(0) >= *ThisTimer->TimeStamp + 2) {
                *ThisTimer->XPressed = false;
                ThirdPerson = false;
                input.ki.dwFlags = 0;
                SendInput(1, &input, sizeof(INPUT));
                Sleep(50);
                input.ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, &input, sizeof(INPUT));
            }
        }
    }

    /* Needs to return a value, and we're not using this */
    return 0;
}


class UevrPlugin : public uevr::Plugin {
public:
    std::time_t m_TimeStamp;
    std::string m_Path;
    TIMER_STRUCT m_Timer;
    bool m_XButtonThirdPerson = true;
    bool m_RightStickUpToggleThirdPerson = false;
    bool m_RightStickDownB = true;
	bool m_ThirdPersonGlide = false;
    bool m_SwapLTRB = false;
    bool m_XPressed;
    UevrPlugin() = default;

    void on_dllmain(HANDLE handle) override {
        ZeroMemory(&m_Timer, sizeof(TIMER_STRUCT));
        m_Timer.XPressed = &m_XPressed;
        m_Timer.TimeStamp = &m_TimeStamp;
        StoreConfigFileLocation(handle);
    }

    void on_initialize() override {
           
        DWORD ThreadId  = 0;
        
        // Start the monitoring thread
        CreateThread(NULL,
                     0,
                     (LPTHREAD_START_ROUTINE)(&TimerCallbackThreadProc),
                     &m_Timer,
                     0,
                     &ThreadId);

        ReadConfig(m_Path);
   }

	//***************************************************************************************************
	// Stores the path and file location of the cvar.txt config file.
	//***************************************************************************************************
	void StoreConfigFileLocation(HANDLE handle) {
		wchar_t wide_path[MAX_PATH]{};
		if (GetModuleFileNameW((HMODULE)handle, wide_path, MAX_PATH)) {
			const auto path = std::filesystem::path(wide_path).parent_path() / "PersonToggle.txt";
			m_Path = path.string(); // change m_Path to a std::string
		}
	}	

    //*******************************************************************************************
    // This is the controller input routine. Everything happens here.
    //*******************************************************************************************
    void on_xinput_get_state(uint32_t* retval, uint32_t user_index, XINPUT_STATE* state) {
        
        static TP_CAUSE Cause = CAUSE_NONE;
 		static bool FirstPerson = true;
        static bool KeyDown = false;
        bool IsInDialogue = false;
        bool IsInCinematic = false;
        static bool XDown = false;
        static bool RightStickUp = false;
        static bool InMenu = false;
        static bool StartKeyDown = false;
		static bool ShouldersDown = false;
        
        INPUT input;
        ZeroMemory(&input, sizeof(INPUT));
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_F4;

        if(KeyDown == true){
            KeyDown = false;
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
        }
        
        if(state != NULL) {
            
            // Check if we are in 3rd person and if so, if we should switch back to first person. 
            if(FirstPerson == false) {
                if(Cause < CAUSE_RIGHT_STICK) {
                    const auto DM = DialogueManager::get_instance();
                    const auto CM = CinematicManager::get_instance();
                    if(DM && DM->is_in_dialogue()) {
                        IsInDialogue = true;
                    }
                    if(CM && CM->is_in_cinematic()) {
                        IsInCinematic = true;
                    }
                    
                    if(IsInCinematic == false && IsInDialogue == false) {
                       KeyDown = true;
                       input.ki.dwFlags = 0;
                       SendInput(1, &input, sizeof(INPUT));
                       FirstPerson = true;
                       Cause = CAUSE_NONE;
                       API::get()->log_info("InDialog=%d, InCinema=%d, In 3rd, switching to 1st person", IsInDialogue, IsInCinematic);
                    }
                } 
                
            }


			// Toggle state based on melee or not.
            if(InMenu == false) {
                if(m_XButtonThirdPerson && FirstPerson == true) {
                    if(state->Gamepad.wButtons & XINPUT_GAMEPAD_X) {
                        if(XDown == false) {
                            XDown = true;
                            m_TimeStamp = std::time(0);
                            m_XPressed = true;
                        }
                    } else {
                        if(XDown == true) {
                            XDown = false;
                        }
                    }
                }
                
                if(m_SwapLTRB) {
                    bool TempRB = (state->Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? true : false;
                    
                    if(state->Gamepad.bLeftTrigger >= 200) {
                        state->Gamepad.wButtons |= (XINPUT_GAMEPAD_RIGHT_SHOULDER);
                    } else {
                        state->Gamepad.wButtons &= ~(XINPUT_GAMEPAD_RIGHT_SHOULDER);
                    }
                    
                    state->Gamepad.bLeftTrigger = (TempRB) ? 255 : 0;
                }
            }
            
			if(m_ThirdPersonGlide == true) {
				if((state->Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) && 
				   (state->Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
					if(ShouldersDown == false && FirstPerson == true) {
						ShouldersDown = true;
					    KeyDown = true;
					    input.ki.dwFlags = 0;
					    SendInput(1, &input, sizeof(INPUT));
					    FirstPerson = false;
					    Cause = CAUSE_RBLB;
					}
				} else {
					if(ShouldersDown == true && FirstPerson == false && Cause == CAUSE_RBLB) {
						ShouldersDown = false;
					    KeyDown = true;
					    input.ki.dwFlags = 0;
					    SendInput(1, &input, sizeof(INPUT));
					    FirstPerson = true;
						Cause = CAUSE_NONE;
					}
				}
			}

            // Check if in conversation and if so, go to 3rd person.
            if(state->Gamepad.wButtons & XINPUT_GAMEPAD_Y) {
                if(FirstPerson == true) {
                    const auto DM = DialogueManager::get_instance();
                    if(DM) {
                        IsInDialogue = DM->is_in_dialogue();
                        if(IsInDialogue) {
                           KeyDown = true;
                           input.ki.dwFlags = 0;
                           SendInput(1, &input, sizeof(INPUT));
                           FirstPerson = false;
                           Cause = CAUSE_DIALOGUE;
                           API::get()->log_info("In Dialogue=%d. Switching to 3rd person.", IsInDialogue);
                        }
                    } 
                }
            }

            // Check if in cinematic (movie) and if so, go to 3rd person.
            if(FirstPerson == true) {
                const auto CM = CinematicManager::get_instance();
                if(CM) {
                    IsInCinematic = CM->is_in_cinematic();
                    if(IsInCinematic) {
                       KeyDown = true;
                       input.ki.dwFlags = 0;
                       SendInput(1, &input, sizeof(INPUT));
                       FirstPerson = false;
                       Cause = CAUSE_CINEMATIC;
                       API::get()->log_info("In Cinematic=%d. Switching to 3rd Person", IsInCinematic);
                    }
                } 
            }
                
            // Right stick B
            if(m_RightStickDownB) {
                if(state->Gamepad.sThumbRY <= -25000) {
                    state->Gamepad.sThumbRY = 0;
                    state->Gamepad.wButtons |= (XINPUT_GAMEPAD_B);
                }
            }
            
            if(m_RightStickUpToggleThirdPerson) {
                if(state->Gamepad.sThumbRY >= 25000 && RightStickUp == false) {
                    state->Gamepad.sThumbRY = 0;
                    RightStickUp = true;
                    
                    input.ki.dwFlags = 0;
                    SendInput(1, &input, sizeof(INPUT));
                    KeyDown = true;
                } else if(state->Gamepad.sThumbRY < 5000) {
                    RightStickUp = false;
                    FirstPerson = !FirstPerson;
                }
            }
            
            // Prevent X from toggling camera and sending a keystroke in the menu where we may
            // be trying to upgrade an item.
            if(InMenu == false) {
                if(state->Gamepad.wButtons & (XINPUT_GAMEPAD_START)) {
                    StartKeyDown = true;
                } else if(StartKeyDown == true) {
                    StartKeyDown = false;
                    InMenu = true;
                }
            } else {
                if(state->Gamepad.wButtons & (XINPUT_GAMEPAD_B)) {
                    InMenu = false;
                } else if(state->Gamepad.wButtons & (XINPUT_GAMEPAD_START)) {
                    StartKeyDown = true;
                } else if(StartKeyDown == true) {
                    StartKeyDown = false;
                    InMenu = false;
                }
            }
                
        }

    }

	//***************************************************************************************************
	// Reads the config file cvars.txt and stores it in a linked list of CVAR_ITEMs.
	//***************************************************************************************************
    void ReadConfig(std::string ConfigFile) {
		std::string Line;
		
		int Length = 0;
		int i = 0;
        int LineNumber = 0;
		size_t Pos = 0;
		
        API::get()->log_info("reading config file %s", ConfigFile.c_str());
		std::ifstream fileStream(ConfigFile.c_str());
		if(!fileStream.is_open()) {
			API::get()->log_info("%s cannot be opened or does not exist, using defaults.", ConfigFile.c_str());
			return;
		}
			
		while (std::getline(fileStream, Line)) {
            LineNumber++;

            Length = static_cast<int>(Line.length());

			if(Line[0] == '#') continue;
			if(Line[0] == ';') continue;
			if(Line[0] == '[') continue;
			if(Line[0] == ' ') continue;
			if(Length < 3) continue;
			
			// Strip  spaces, carriage returns from line.
			Pos = Line.find_last_not_of(" \r\n");
			if(Pos != std::string::npos) {
				Line.erase(Pos + 1);
			}

			Pos = Line.find('=');
			if(Pos == std::string::npos) {
				continue;
			}

			API::get()->log_info("Processing config line: %s", Line.c_str());   
			
            std::string ConfigLine = Line.substr(0, Pos);
            std::string ConfigValue = Line.substr(Pos + 1, MAX_ELEMENT_LEN);
            
            if(ConfigLine == "XButtonThirdPerson") {
                if(ConfigValue == "1") m_XButtonThirdPerson = 1;
                else m_XButtonThirdPerson = 0;
            }

            if(ConfigLine == "RightStickUpToggleThirdPerson") {
                if(ConfigValue == "1") m_RightStickUpToggleThirdPerson = 1;
                else m_RightStickUpToggleThirdPerson = 0;
            }
			
            if(ConfigLine == "RightStickDownB") {
                if(ConfigValue == "1") m_RightStickDownB = 1;
                else m_RightStickDownB = 0;
            }
			
            if(ConfigLine == "ThirdPersonGlide") {
                if(ConfigValue == "1") m_ThirdPersonGlide = 1;
                else m_ThirdPersonGlide = 0;
            }

            if(ConfigLine == "SwapLTRB") {
                if(ConfigValue == "1") m_SwapLTRB = 1;
                else m_SwapLTRB = 0;
            }

			API::get()->log_info("PersonToggle.txt Added entry command: %s=%s", ConfigLine.c_str(), ConfigValue.c_str());
		}		
		
		fileStream.close();
	}    
};
    
// Actually creates the plugin. Very important that this global is created.
// The fact that it's using std::unique_ptr is not important, as long as the constructor is called in some way.
std::unique_ptr<UevrPlugin> g_plugin{new UevrPlugin()};

void DebugPrint(char* Format, ...)
{
  char FormattedMessage[512];    
  va_list ArgPtr = NULL;  
  
  /* Generate the formatted debug message. */        
  va_start(ArgPtr, Format);        
  vsprintf(FormattedMessage, Format, ArgPtr);        
  va_end(ArgPtr); 

  OutputDebugString(FormattedMessage);
}

