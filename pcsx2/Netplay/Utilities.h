#pragma once
#include "App.h"
#include "EmulatorState.h"
#include <vector>
#include <boost/shared_ptr.hpp>

class Utilities
{
public:
	typedef std::vector<u8> block_type;

	// TODO: use rvalue references here
	static block_type ReadMCD(uint port, uint slot);
	static void WriteMCD(uint port, uint slot, const block_type& block);
	static bool Compress(const Utilities::block_type& uncompressed,
		Utilities::block_type& compressed);
	static bool Uncompress(const Utilities::block_type& compressed,
		Utilities::block_type& uncompressed);
	static size_t GetMCDSize(uint port, uint slot);
	static boost::shared_ptr<EmulatorSyncState> GetSyncState();
	static wxString GetDiscNameById(const wxString& id);
	static wxString GetCurrentDiscId();
	static wxString GetCurrentDiscName();

	static void DispatchEvent();
	static void ExecuteOnMainThread(const std::function<void()>& evt);
private:
	static std::function<void()> _dispatch_event;
};
