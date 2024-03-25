#pragma once

#include "uevr/API.hpp"

using namespace uevr;

class DialogueManager : public API::UObject
{
  public:
	using API::UObject::get_full_name;

	static API::UClass *static_class()
	{
		static API::UClass *result = nullptr;
		if(!result) {
			result = API::get()->find_uobject<API::UClass>(L"Class /Script/O2.DialogueManager");
		}
		return result;
	}

	static DialogueManager *get_instance()
	{
		auto klass = DialogueManager::static_class();
        DialogueManager* Object = nullptr;
        
		if(klass) {
            std::vector<DialogueManager*> List = klass->get_objects_matching<DialogueManager>();
            for(size_t i = 0; i < List.size(); i++) {
                Object = List[i];
                
                std::wstring ObjName = Object->get_full_name();
                API::get()->log_info("DialogueManager: Object %d of %d, Object name: %ls",i, List.size(), ObjName.c_str());
                // Skip anything with GEN_VARIABLE
                if(ObjName.size() > 0 && ObjName.find(L"GEN_VARIABLE") == std::wstring::npos) {
                    API::get()->log_info("DialogueManager: returning instance %d, %ls", i, ObjName.c_str());
                    break;
                }
            }
			return Object;
		}
		return nullptr;
	}

	bool is_in_dialogue()
	{
		static const auto func = DialogueManager::static_class()->find_function(L"IsInDialogue");
		if(!func) {
			API::get()->log_error("DialogueManager::IsInDialogue not found");
			return false;
		} 
        
		struct
		{
			bool res;
		} params{0};

		process_event(func, &params);

		return params.res;
	}

};