#pragma once

#include "uevr/API.hpp"

using namespace uevr;

class CinematicManager : public API::UObject
{
  public:
	using API::UObject::get_full_name;

	static API::UClass *static_class()
	{
		static API::UClass *result = nullptr;
		if(!result) {
			result = API::get()->find_uobject<API::UClass>(L"Class /Script/O2.CinematicManager");
		}
		return result;
	}

	static CinematicManager *get_instance()
	{
		auto klass = CinematicManager::static_class();
        CinematicManager* Object = nullptr;
        
		if(klass) {
            std::vector<CinematicManager*> List = klass->get_objects_matching<CinematicManager>();
            for(size_t i = 0; i < List.size(); i++) {
                Object = List[i];
                
                std::wstring ObjName = Object->get_full_name();
                //API::get()->log_info("CinematicManager: Object %d of %d, Object name: %ls",i, List.size(), ObjName.c_str());
                // Skip anything with GEN_VARIABLE
                if(ObjName.size() > 0 && ObjName.find(L"GEN_VARIABLE") == std::wstring::npos) {
                    //API::get()->log_info("CinematicManager: returning instance %d, %ls", i, ObjName.c_str());
                    break;
                }
            }
			return Object;
		}
		return nullptr;
	}

	bool is_in_cinematic()
	{
		static const auto func = CinematicManager::static_class()->find_function(L"IsCinematicCurrentlyPlaying");
		if(!func) {
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