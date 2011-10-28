#include "PrecompiledHeader.h"
#include "ReplayPlugin.h"
#include "Replay.h"
#include "Utilities.h"
#include "App.h"
#include <sstream>

class ReplayPlugin : public IReplayPlugin
{
public:
	void Init()
	{
		Utilities::SaveSettings();
		Utilities::ResetSettingsToSafeDefaults();
	}
	void Open()
	{
		try
		{
			if(_replay.LoadFromFile(g_Conf->Replay.FilePath))
			{
				_replay.Mode(Playback);
				auto state = Utilities::GetSyncState();
				if(state)
				{
					if(memcmp(_replay.SyncState().biosVersion, state->biosVersion, sizeof(state->biosVersion)))
					{
						std::stringstream ss;
						ss << "REPLAY: Incompatible BIOS detected. Please use ";
						ss.write(_replay.SyncState().biosVersion, sizeof(state->biosVersion));
						Console.Error(ss.str().c_str());
						Stop();
						return;
					}
					if(memcmp(_replay.SyncState().discId, state->discId, sizeof(state->discId)))
					{
						size_t myDiscIdLen = sizeof(state->discId);
						size_t replayDiscIdLen = sizeof(_replay.SyncState().discId);
						for(size_t i = 0; i < myDiscIdLen; i++)
						{
							if(state->discId[i] == 0)
							{
								myDiscIdLen = i;
								break;
							}
						}
						for(size_t i = 0; i < replayDiscIdLen; i++)
						{
							if(_replay.SyncState().discId[i] == 0)
							{
								replayDiscIdLen = i;
								break;
							}
						}

						wxString myDiscId(state->discId, wxConvUTF8, myDiscIdLen);
						wxString replayDiscId(_replay.SyncState().discId, wxConvUTF8, replayDiscIdLen);

						wxString msg = wxString(wxT("REPLAY: Incompatible disc detected: ")) + 
								Utilities::GetDiscNameById(myDiscId) + wxString(wxT(" instead of ")) + Utilities::GetDiscNameById(replayDiscId);
						Stop();
						Console.Error(msg);
						return;
					}
				}
				_mcd_backup = Utilities::ReadMCD(0,0);
				Utilities::WriteMCD(0,0,_replay.Data());
			}
			else
			{
				Stop();
				Console.Error("REPLAY: file is corrupted or of the older version.");
			}
		}
		catch(std::exception& e)
		{
			Stop();
			Console.Error("REPLAY: %s", e.what());
		}
	}
	void Close()
	{
		_replay = Replay();
		Utilities::RestoreSettings();
		if(_mcd_backup.size())
		{
			Utilities::WriteMCD(0,0,_mcd_backup);
			_mcd_backup.clear();
		}
	}
	u8 HandleIO(int side, int index, u8 value)
	{
		if(_replay.Pos() < _replay.Length())
			return _replay.Read(side).input[index];
		else
			return value;
	}
	void NextFrame()
	{
		if(!_replay.Pos())
			Console.WriteLn(Color_StrongGreen, "REPLAY: starting playback. Press F4 to fast-forward.");

		if(_replay.Pos() >= _replay.Length())
		{
			Stop();
			Console.WriteLn(Color_StrongGreen, "REPLAY: playback ended.");
		}
		else
			_replay.NextFrame();
	}
	void AcceptInput(int){}
	void Stop()
	{
		Utilities::ExecuteOnMainThread([&]() {
			CoreThread.Reset();
			UI_EnableEverything();
		});
	}
protected:
	Replay _replay;
	Utilities::block_type _mcd_backup;
};

IReplayPlugin* IReplayPlugin::instance = 0;

IReplayPlugin& IReplayPlugin::GetInstance()
{
	if(!instance)
		instance = new ReplayPlugin();
	return *instance;
}