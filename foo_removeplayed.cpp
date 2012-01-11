#define VERSION ("1.4.0")

#include "../_sdk/foobar2000/SDK/foobar2000.h"
#include "../_sdk/foobar2000/helpers/helpers.h"


DECLARE_COMPONENT_VERSION( "Remove played Files", VERSION, 
   "Removes played files from playlist\n"
   "By Chronial\n"
   __DATE__ " - " __TIME__ )

metadb_handle_ptr g_current_track;
// {D8882A57-C32C-47b1-AE3B-016CCAA05B7C}
static const GUID guid_removeplayed_enabled = { 0xd8882a57, 0xc32c, 0x47b1, { 0xae, 0x3b, 0x1, 0x6c, 0xca, 0xa0, 0x5b, 0x7c } };
static cfg_bool cfg_removeplayed_enabled(guid_removeplayed_enabled, true);

// {2C4FDA9F-6F29-488b-A9C4-7E341A9FC81F}
static const GUID guid_removeskipped_enabled = { 0x2c4fda9f, 0x6f29, 0x488b, { 0xa9, 0xc4, 0x7e, 0x34, 0x1a, 0x9f, 0xc8, 0x1f } };
static cfg_bool cfg_removeskipped_enabled(guid_removeskipped_enabled, true);

class my_initquit : public initquit
{
   void on_init() 
   {
   }

   void on_quit() 
   {
      g_current_track = 0;      
   }
};

static service_factory_single_t<my_initquit> g_my_initquit;

class remove_callback : public main_thread_callback {
public:
	t_size playlist;
	t_size index;
	void callback_run(){
		static_api_ptr_t<playlist_manager> pm;
		const bit_array_one del(index);
		pm->playlist_remove_items(playlist,del);
	}
};

class my_play_callback : public play_callback_static {
public:
	my_play_callback(){
		trackSkipped = false;
	}
private:
	double lastTime;
	bool trackSkipped;
	void removePlayedSong(){
		if ((!trackSkipped && cfg_removeplayed_enabled) || (trackSkipped && cfg_removeskipped_enabled)){
			t_size playlist;
			t_size songIndex;

			static_api_ptr_t<playlist_manager> pm;
			// this works in a strange way: This actually gets the info of
			// the previous track and not the new one (when called from on_playback_new_track).
			if (pm->get_playing_item_location(&playlist,&songIndex)){
				//if ((lastTime + 1) > g_current_track->get_length()){
				if (g_current_track != 0){
					static_api_ptr_t<main_thread_callback_manager> cm;
					service_ptr_t<remove_callback> callback = new service_impl_t<remove_callback>;
					callback->index = songIndex;
					callback->playlist = playlist;
					cm->add_callback(callback);
				}
			}
		}
		trackSkipped = false;
	}
public:
	//! Controls which methods your callback wants called; returned value should not change in run time, you should expect it to be queried only once (on startup). See play_callback::flag_* constants.
	virtual unsigned get_flags(){
		return play_callback::flag_on_playback_new_track
			//| play_callback::flag_on_playback_time
			| play_callback::flag_on_playback_stop;
	};

	//! Playback advanced to new track.
	virtual void on_playback_new_track(metadb_handle_ptr p_track){
		removePlayedSong();
		g_current_track = p_track;
		//lastTime = 0;
	};

	//! Playback stopped.
	virtual void on_playback_stop(play_control::t_stop_reason p_reason){
		// was last song in playlist
		if (p_reason == play_control::stop_reason_eof){
			trackSkipped = false;
			removePlayedSong();
		} else if (p_reason == play_control::stop_reason_starting_another){
			trackSkipped = true;
			removePlayedSong();
		} else {
			g_current_track = 0;
		}
	};

	//! Called every second, for time display
	virtual void on_playback_time(double p_time){
		/*static_api_ptr_t<playback_control> pc;
		lastTime = p_time;*/
	};


	virtual void on_playback_starting(play_control::t_track_command p_command,bool p_paused){};

	//! User has seeked to specific time.
	virtual void on_playback_seek(double p_time){};
	//! Called on pause/unpause.
	virtual void FB2KAPI on_playback_pause(bool p_state){};
	//! Called when currently played file gets edited.
	virtual void on_playback_edited(metadb_handle_ptr p_track){};
	//! Dynamic info (VBR bitrate etc) change.
	virtual void on_playback_dynamic_info(const file_info & p_info){};
	//! Per-track dynamic info (stream track titles etc) change. Happens less often than on_playback_dynamic_info().
	virtual void on_playback_dynamic_info_track(const file_info & p_info){};
	//! User changed volume settings. Possibly called when not playing.
	//! @param p_new_val new volume level in dB; 0 for full volume.
	virtual void FB2KAPI on_volume_change(float p_new_val){};
};

static service_factory_single_t<my_play_callback> g_play_callback;


class my_mainmenu_commands : public mainmenu_commands {
	// Return the number of commands we provide.
	virtual t_uint32 get_command_count() {
		return 2;
	}

	// All commands are identified by a GUID.
	virtual GUID get_command(t_uint32 p_index) {
		// {85C4F7DD-9652-4c3f-BBA7-51D9FDEE1E5D}
		static const GUID guid_enable_removecurrent = { 0x85c4f7dd, 0x9652, 0x4c3f, { 0xbb, 0xa7, 0x51, 0xd9, 0xfd, 0xee, 0x1e, 0x5d } };

		// {57829D63-8E55-485a-9E5B-E56437FCBCB3}
		static const GUID guid_enable_removeskipped = { 0x57829d63, 0x8e55, 0x485a, { 0x9e, 0x5b, 0xe5, 0x64, 0x37, 0xfc, 0xbc, 0xb3 } };

		if (p_index == 0)
			return guid_enable_removecurrent;
		else if (p_index == 1)
			return guid_enable_removeskipped;
		return pfc::guid_null;
	}

	// Set p_out to the name of the n-th command.
	// This name is used to identify the command and determines
	// the default position of the command in the menu.
	virtual void get_name(t_uint32 p_index, pfc::string_base & p_out) {
		if (p_index == 0)
			p_out = "Remove played tracks";
		else if (p_index == 1)
			p_out = "Remove skipped tracks";
	}

	// Set p_out to the description for the n-th command.
	virtual bool get_description(t_uint32 p_index, pfc::string_base & p_out) {
		if (p_index == 0)
			p_out = "Removes tracks from the Playlist once they've been played";
		else if (p_index == 1)
			p_out = "Removes tracks from the Playlist if you skip them";
		else
			return false;
		return true;
	}

	// Every set of commands needs to declare which group it belongs to.
	virtual GUID get_parent()
	{
		return mainmenu_groups::playback_etc;
	}

	// Execute n-th command.
	// p_callback is reserved for future use.
	virtual void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) {
		if (p_index == 0) {
			cfg_removeplayed_enabled = !cfg_removeplayed_enabled;
		} else if (p_index == 1) {
			cfg_removeskipped_enabled = !cfg_removeskipped_enabled;
		}
	}

	// The standard version of this command does not support checked or disabled
	// commands, so we use our own version.
	virtual bool get_display(t_uint32 p_index, pfc::string_base & p_text, t_uint32 & p_flags)
	{
		p_flags = 0;
		if (is_checked(p_index))
			p_flags |= flag_checked;
		get_name(p_index,p_text);
		return true;
	}

	// Return whether the n-th command is checked.
	bool is_checked(t_uint32 p_index) {
		if (p_index == 0)
			return cfg_removeplayed_enabled;
		else if (p_index == 1)
			return cfg_removeskipped_enabled;
		return false;
	}
};

// We need to create a service factory for our menu item class,
// otherwise the menu commands won't be known to the system.
static mainmenu_commands_factory_t<my_mainmenu_commands> foo_menu;