#include "PrecompiledHeader.h"
#include "Utilities.h"
#include "zlib\zlib.h"
#include "ps2/BiosTools.h"
#include "Elfheader.h"
#include "CDVD/CDVD.h"
#include "AppGameDatabase.h"

size_t Utilities::GetMCDSize(uint port, uint slot)
{
	PS2E_McdSizeInfo info;
	SysPlugins.McdGetSizeInfo(port,slot,info);
	return info.McdSizeInSectors*(info.SectorSize + info.EraseBlockSizeInSectors);
}

Utilities::block_type Utilities::ReadMCD(uint port, uint slot)
{
	block_type mcd;
	size_t size = GetMCDSize(port, slot);
	mcd.resize(size);
	SysPlugins.McdRead(0,0, mcd.data(), 0, size);
	return mcd;
}
void Utilities::WriteMCD(uint port, uint slot, const Utilities::block_type& block)
{
	size_t size = GetMCDSize(port, slot);
	if(block.size() != size)
		throw std::exception("invalid block size");
	for(size_t p = 0; p < size; p+= 528*16)
		SysPlugins.McdEraseBlock(0,0,p);

	SysPlugins.McdSave(port,slot, block.data(), 0, size);
}
bool Utilities::Compress(const Utilities::block_type& uncompressed,
	Utilities::block_type& compressed)
{
	uLongf size = uncompressed.size();
	compressed.resize(size);
	int r = compress2(compressed.data(), &size, uncompressed.data(), size, Z_BEST_COMPRESSION);
	if(r != Z_OK)
		return false;
	if(size < compressed.size())
		compressed.resize(size);
	return true;
}
bool Utilities::Uncompress(const Utilities::block_type& compressed,
	Utilities::block_type& uncompressed)
{
	uLongf size = uncompressed.size();
	int r = uncompress((Bytef*)uncompressed.data(), &size, (Bytef*)compressed.data(), compressed.size());
	if(r != Z_OK)
		return false;
	if(size < uncompressed.size())
		uncompressed.resize(size);
	return true;
}

void Utilities::DispatchEvent()
{
	if(_dispatch_event)
		_dispatch_event();
}
void Utilities::ExecuteOnMainThread(const std::function<void()>& evt)
{
	if(!_dispatch_event)
		_dispatch_event = evt;
	if (!wxGetApp().Rpc_TryInvoke( DispatchEvent ))
		ExecuteOnMainThread(evt);
	if(_dispatch_event)
		_dispatch_event = std::function<void()>();
}

wxString Utilities::GetCurrentDiscId()
{
	cdvdReloadElfInfo();
	auto diskId = SysGetDiscID();
	if(diskId.Len() == 0)
	{
		ExecuteOnMainThread([&]() {
			Console.Error("Unable to get disc id.");
		});
	}
	return diskId;
}

boost::shared_ptr<EmulatorSyncState> Utilities::GetSyncState()
{
	boost::shared_ptr<EmulatorSyncState> syncState(new EmulatorSyncState());

	cdvdReloadElfInfo();
	auto diskId = GetCurrentDiscId();
	if(diskId.Len() == 0)
	{
		syncState.reset();
		return syncState;
	}

	memset(syncState->discId, 0, sizeof(syncState->discId));
	memcpy(syncState->discId, diskId.ToAscii().data(), 
		diskId.length() > sizeof(syncState->discId) ? 
		sizeof(syncState->discId) : diskId.length());

	wxString biosDesc;
	if(!IsBIOS(g_Conf->EmuOptions.BiosFilename.GetFullPath(), biosDesc))
	{
		ExecuteOnMainThread([&]() {
			Console.Error("Unable to read BIOS information.");
		});
		syncState.reset();
		return syncState;
	}
	memset(syncState->biosVersion, 0, sizeof(syncState->biosVersion));
	memcpy(syncState->biosVersion, biosDesc.ToAscii().data(), 
		biosDesc.length() > sizeof(syncState->biosVersion) ? 
		sizeof(syncState->biosVersion) : biosDesc.length());
	return syncState;
}


wxString Utilities::GetDiscNameById(const wxString& id)
{
	wxString discName;
	Game_Data game;
	IGameDatabase* GameDB = AppHost_GetGameDatabase();
	if(GameDB && GameDB->findGame(game, id))
		discName = game.getString("Name") + wxT(" (") + game.getString("Region") + wxT(")");
	else
		discName = id;
	return discName;
}
wxString Utilities::GetCurrentDiscName()
{
	return GetDiscNameById(GetCurrentDiscId());
}

//
	
std::function<void()> Utilities::_dispatch_event = std::function<void()>();