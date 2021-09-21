local const char* last_cmd_desc;
#define CMDSTART(name, desc) last_cmd_desc = desc; auto deshi__cmd__##name = [](array<cstring>& args) -> void
#define CMDEND(name, ...) ; Cmd::Add(deshi__cmd__##name, #name, last_cmd_desc, {__VA_ARGS__});

void AddAtmosCommands(){
	CMDSTART(save_level, "Saves the current editor state into a level"){
		AtmoAdmin->SaveLevel(args[0]);
	}CMDEND(save_level, CmdArgument_String);
	
	CMDSTART(load_level, "Loads a level after clearing the current editor state"){
		AtmoAdmin->LoadLevel(args[0]);
	}CMDEND(load_level, CmdArgument_String);
}

#undef CMDSTART
#undef CMDEND