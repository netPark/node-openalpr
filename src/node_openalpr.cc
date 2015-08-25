#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <uv.h>
#include "nan.h"
#include <alpr.h>
#include <config.h>
#include <string.h>
#include <fstream>
#include <list>
#include <vector>

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif

using namespace v8;

uv_mutex_t 		listMutex;
bool running = false;

class LPRQueueItem 
{
	public:	
		char *path;
		char *state;
		std::string prewarp;
		bool detectRegion = false;
		std::vector <alpr::AlprRegionOfInterest> regions;
		Nan::Callback *callback;
};

class LPR {
	public:
		LPR (std::string configFile = "", std::string runtimePath = "") {
			this->openalpr = new alpr::Alpr ("us", configFile, runtimePath);
			this->openalpr->setTopN (10);
			this->config = this->openalpr->getConfig ();
		}
		
		~LPR () {
			delete this->openalpr;
		}
		
		bool isLoaded () {
			return this->openalpr->isLoaded ();
		}
		
		alpr::AlprResults recognize (LPRQueueItem *queueItem) {
			this->openalpr->setDefaultRegion (queueItem->state);
			this->openalpr->setDetectRegion (queueItem->detectRegion);
			this->config->prewarp = queueItem->prewarp;
			
			std::ifstream ifs (queueItem->path, std::ios::binary|std::ios::ate);
			std::ifstream::pos_type pos = ifs.tellg ();
			std::vector<char>  buffer(pos);
			ifs.seekg(0, std::ios::beg);
			ifs.read(&buffer[0], pos);
			
			if (queueItem->regions.size ()) {
				return this->openalpr->recognize (buffer, queueItem->regions);
			}
			else {
				return this->openalpr->recognize (buffer);
			}
		}
		
		bool isWorking () {
			return working;
		}
		
		void isWorking (bool working) {
			this->working = working;
		}
		
	private:
		alpr::Alpr *openalpr;
		alpr::Config *config;
		bool working = false;
};

class LPRWorker : public Nan::AsyncWorker 
{
	public:
		LPRWorker (LPR *lpr, LPRQueueItem *queueItem) : Nan::AsyncWorker (queueItem->callback) {
			this->lpr = lpr;
			this->lpr->isWorking (true);
			this->queueItem = queueItem;
		}
		
		~LPRWorker () {
			delete this->queueItem;
		}
		
		void Execute () {
			alpr::AlprResults results = lpr->recognize (queueItem);
			output = alpr::Alpr::toJson (results);
		}
		
		void HandleOKCallback () {
			Nan::HandleScope scope;
			v8::Local<v8::Value> argv[] = {
				Nan::Null (),
				Nan::New<String>(output.c_str ()).ToLocalChecked ()
			};
			
			lpr->isWorking (false);
			if (!running) {
				delete lpr;
			}
			
			callback->Call (2, argv);
		}
		
	private:
		LPR *lpr;
		LPRQueueItem *queueItem;
		std::string output;
};

static std::list<LPR *> instances;
static std::list<LPRQueueItem *> queue;

char *get (v8::Local<v8::Value> value)
{
	if (value->IsString ()) {
		v8::String::Utf8Value string (value);
		char *str = (char *)malloc (string.length () + 1);
		strcpy (str, *string);
		return str;
	}
	
	return "";
}

NAN_METHOD (Start)
{
	running = true;
	char *config_path = get (info[0]);
	char *runtime_path = get (info[1]);
	int instancesCount = info[2]->NumberValue ();
	
	// Create a list of instances, if any fail to load return false
	for (int i = 0; i < instancesCount; i++) {
		LPR *lpr = new LPR (config_path, runtime_path);
		if (lpr->isLoaded () == false) {
			info.GetReturnValue ().Set (false);
		}
		
		instances.push_back (lpr);
	}

	uv_mutex_init (&listMutex);
	info.GetReturnValue ().Set (true);
}

NAN_METHOD (Stop)
{
	running = false;

	// Clear out the queue (have to pull and item and delete it to get rid of it correctly)
	while (queue.size () > 0) {
		LPRQueueItem *item = queue.front ();
		delete item;
		
		queue.pop_front ();
	}
	
	instances.clear ();
	info.GetReturnValue ().Set (true);
}

NAN_METHOD (GetVersion)
{
	std::string version = alpr::Alpr::getVersion ();	
	info.GetReturnValue ().Set <String> (Nan::New <String> (version).ToLocalChecked ());
}

NAN_METHOD (IdentifyLicense)
{
	// Lock our mutex so we can safetly modify our lists
	uv_mutex_lock (&listMutex);

	// Settings	
	char *path = get (info[0]);
	char *state = get (info[1]);
	char *prewarp = get (info[2]);
	bool detectRegion = info[3]->BooleanValue ();
	Local<Array> regionsArray = info[4].As<Array> ();
	Nan::Callback *callback = new Nan::Callback (info[5].As<Function>());
	
	std::vector<alpr::AlprRegionOfInterest> regions;
	for (int i = 0; i < regionsArray->Length (); i++) {
		Local<Array> regionValues = Local<Array>::Cast (regionsArray->Get (i));
		int x = regionValues->Get (0)->Uint32Value ();
		int y = regionValues->Get (1)->Uint32Value ();
		int width = regionValues->Get (2)->Uint32Value ();
		int height = regionValues->Get (3)->Uint32Value ();
		regions.push_back (alpr::AlprRegionOfInterest (x, y, width, height));
	}
		
	LPRQueueItem *item = new LPRQueueItem ();
	item->path = path;
	item->state = state;
	item->prewarp = prewarp;
	item->detectRegion = detectRegion;
	item->regions = regions;
	item->callback = callback;
	
	for (auto &i : instances) {
		if (!i->isWorking ()) {
			Nan::AsyncQueueWorker (new LPRWorker (i, item));
			
			// Unlock mutexes - we're done changing the lists
			uv_mutex_unlock (&listMutex);
			info.GetReturnValue ().Set <String> (Nan::New <String> ("working").ToLocalChecked ());
			return;
		}
	}
	
	// If we reach here, we need to queue the image for later processing
	queue.push_back (item);
	
	// Unlock mutexes - we're done changing the lists
	uv_mutex_unlock (&listMutex);
	info.GetReturnValue ().Set <String> (Nan::New <String> ("queued").ToLocalChecked ());
}

NAN_METHOD (CheckQueue)
{
	int started = 0;
	
	uv_mutex_lock (&listMutex);
	
	// Verify we have a queue, then start trying to associate them with an instance
	if (queue.size () > 0) {
		for (auto &i : instances) {
			if (!i->isWorking ()) {
				LPRQueueItem *item = queue.front ();
				Nan::AsyncQueueWorker (new LPRWorker (i, item));
				
				// Delete items from the front for a FIFO operation
				queue.pop_front ();
				started++;
				
				if (queue.size () == 0) {
					break;
				}
			}
		}
	}
	
	uv_mutex_unlock (&listMutex);
	info.GetReturnValue ().Set (started);
}

NAN_MODULE_INIT (InitAll) {
	Nan::Set (target, Nan::New<String>("Start").ToLocalChecked (), Nan::GetFunction (Nan::New<FunctionTemplate>(Start)).ToLocalChecked ());
	Nan::Set (target, Nan::New<String>("Stop").ToLocalChecked (), Nan::GetFunction (Nan::New<FunctionTemplate>(Stop)).ToLocalChecked ());
	Nan::Set (target, Nan::New<String>("GetVersion").ToLocalChecked (), Nan::GetFunction (Nan::New<FunctionTemplate>(GetVersion)).ToLocalChecked ());
	Nan::Set (target, Nan::New<String>("IdentifyLicense").ToLocalChecked (), Nan::GetFunction (Nan::New<FunctionTemplate>(IdentifyLicense)).ToLocalChecked ());
	Nan::Set (target, Nan::New<String>("CheckQueue").ToLocalChecked (), Nan::GetFunction (Nan::New<FunctionTemplate>(CheckQueue)).ToLocalChecked ());
}

NODE_MODULE(node_openalpr, InitAll);
