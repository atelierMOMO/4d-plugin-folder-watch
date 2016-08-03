/* --------------------------------------------------------------------------------
 #
 #	4DPlugin.cpp
 #	source generated by 4D Plugin Wizard
 #	Project : Folder Watch II
 #	author : miyako
 #	2015/06/16
 #
 # --------------------------------------------------------------------------------*/


#include "4DPluginAPI.h"
#include "4DPlugin.h"

namespace FW2
{
    process_name_t MONITOR_PROCESS_NAME = (PA_Unichar *)"$\0F\0O\0L\0D\0E\0R\0_\0W\0A\0T\0C\0H\0_\0I\0I\0\0\0";
				process_number_t MONITOR_PROCESS_ID = 0;
    process_stack_size_t MONITOR_PROCESS_STACK_SIZE = 0;
    method_id_t CALLBACK_METHOD_ID = 0;
				bool MONITOR_PROCESS_SHOULD_TERMINATE;
				C_TEXT LISTENER_METHOD;
				C_TEXT WATCH_METHOD;
				NSTimeInterval MONITOR_LATENCY;
				FSEventStreamRef MONITOR_STREAM;
				ARRAY_TEXT WATCH_PATHS;
				ARRAY_TEXT WATCH_PATHS_POSIX;
				std::vector<CUTF16String>CALLBACK_EVENT_PATHS;
				std::vector<NSTimeInterval>CALLBACK_EVENT_IDS;
				std::vector<CUTF16String>CALLBACK_EVENT_FLAGS;
}

void generateUuid(C_TEXT &returnValue)
{
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
    returnValue.setUTF16String([[[NSUUID UUID]UUIDString]stringByReplacingOccurrencesOfString:@"-" withString:@""]);
#else
    CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
    NSString *uuid_str = (NSString *)CFUUIDCreateString(kCFAllocatorDefault, uuid);
    returnValue.setUTF16String([uuid_str stringByReplacingOccurrencesOfString:@"-" withString:@""]);
#endif
}

void gotEvent(FSEventStreamRef stream, 
                 void *callbackInfo, 
                 size_t numEvents, 
                 void *eventPaths, 
                 const FSEventStreamEventFlags eventFlags[], 
                 const FSEventStreamEventId eventIds[]
                 ) {
                 
	NSArray *paths = (NSArray *)(CFArrayRef)eventPaths;
	NSMutableString *pathsString = [[NSMutableString alloc]init];
	NSMutableArray *flags = [[NSMutableArray alloc]init];

	for(uint32_t i = 0; i < [paths count] ; ++i)
	{
		if(i)
		{
			[pathsString appendString:@"\n"];
		}
		[flags addObject:[NSNumber numberWithInteger:eventFlags[i]]];
		NSURL *u = (NSURL *)CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef)[paths objectAtIndex:i], kCFURLPOSIXPathStyle, false);
		if(u)
		{
			NSString *path = (NSString *)CFURLCopyFileSystemPath((CFURLRef)u, kCFURLHFSPathStyle);
			[pathsString appendString:(NSString *)path];
			[path release];
		}
	}
	
	NSString *flagsString = [flags componentsJoinedByString:@","];
	[flags release];
	
	uint32_t len = [pathsString length];
	uint32_t size = (len * sizeof(PA_Unichar)) + sizeof(PA_Unichar);
	std::vector<uint8_t> buf(size);	
	CUTF16String event_path;
	
	if([pathsString getCString:(char *)&buf[0] maxLength:size encoding:NSUnicodeStringEncoding])
	{
		event_path = CUTF16String((const PA_Unichar *)&buf[0], len);
		len = [flagsString length];
		size = (len * sizeof(PA_Unichar)) + sizeof(PA_Unichar);
		buf.resize(size);
		CUTF16String event_flag;
		if([flagsString getCString:(char *)&buf[0] maxLength:size encoding:NSUnicodeStringEncoding])
		{
				event_flag = CUTF16String((const PA_Unichar *)&buf[0], len);
				FW2::CALLBACK_EVENT_PATHS.push_back(event_path);
				FW2::CALLBACK_EVENT_FLAGS.push_back(event_flag);
				FW2::CALLBACK_EVENT_IDS.push_back([[NSDate date]timeIntervalSince1970]);
		}
	}
	
	[pathsString release];
    
	if(event_path.size())
		listenerLoopExecute();
}

#pragma mark -

bool IsProcessOnExit()
{
    C_TEXT name;
    PA_long32 state, time;
    PA_GetProcessInfo(PA_GetCurrentProcessNumber(), name, &state, &time);
    CUTF16String procName(name.getUTF16StringPtr());
    CUTF16String exitProcName((PA_Unichar *)"$\0x\0x\0\0\0");
    return (!procName.compare(exitProcName));
}

void OnStartup()
{

}

void OnCloseProcess()
{
    if(IsProcessOnExit())
				{
        PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopFinish, NULL); 
    }
}

#pragma mark -

void listenerLoop()
{
    FW2::MONITOR_PROCESS_SHOULD_TERMINATE = false;
        
    while(!FW2::MONITOR_PROCESS_SHOULD_TERMINATE)
    { 
        PA_YieldAbsolute();
        
								while(FW2::CALLBACK_EVENT_IDS.size())
								{
            C_TEXT processName;
            generateUuid(processName);
            PA_NewProcess((void *)listenerLoopExecuteMethod, 
                          FW2::MONITOR_PROCESS_STACK_SIZE, 
                          (PA_Unichar *)processName.getUTF16StringPtr());
												
												if(FW2::MONITOR_PROCESS_SHOULD_TERMINATE)
																break;
								}
								
        if(!FW2::MONITOR_PROCESS_SHOULD_TERMINATE)
								{
            PA_FreezeProcess(PA_GetCurrentProcessNumber());  
        }else{
            FW2::MONITOR_PROCESS_ID = 0;
        }
    }
    PA_KillProcess();
}

void listenerLoopStart()
{
    if(!FW2::MONITOR_PROCESS_ID)
				{
        FW2::MONITOR_PROCESS_ID = PA_NewProcess((void *)listenerLoop, 
                                                         FW2::MONITOR_PROCESS_STACK_SIZE, 
                                                         FW2::MONITOR_PROCESS_NAME);  
        
#if VERSIONMAC  
#ifndef kFSEventStreamCreateFlagFileEvents
#define kFSEventStreamCreateFlagFileEvents 0x00000010
#endif
        uint32_t i, length = FW2::WATCH_PATHS_POSIX.getSize();
        NSMutableArray *paths = [[NSMutableArray alloc]initWithCapacity:length];
        
        for(i = 0; i < length; ++i){
            NSString *path = FW2::WATCH_PATHS_POSIX.copyUTF16StringAtIndex(i);
            if([path length]){
                [paths addObject:path];
            }
            [path release];
        } 
        
        FSEventStreamContext context = {0, NULL, NULL, NULL, NULL};
        NSTimeInterval latency = FW2::MONITOR_LATENCY;
        
        FW2::MONITOR_STREAM = FSEventStreamCreate(NULL,
            (FSEventStreamCallback)gotEvent,
            &context,
            (CFArrayRef)paths,
            kFSEventStreamEventIdSinceNow,
            (CFAbsoluteTime)latency,
            kFSEventStreamCreateFlagUseCFTypes 
						| kFSEventStreamCreateFlagFileEvents
            | kFSEventStreamCreateFlagNoDefer
            | kFSEventStreamCreateFlagIgnoreSelf
        );
          
        FSEventStreamScheduleWithRunLoop(FW2::MONITOR_STREAM, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        FSEventStreamStart(FW2::MONITOR_STREAM);
        
        [paths release];
#else
        
#endif         
    }
}

void listenerLoopFinish()
{

    if(FW2::MONITOR_PROCESS_ID)
				{
        //uninstall handler
#if VERSIONMAC  
        FSEventStreamStop(FW2::MONITOR_STREAM);
        FSEventStreamUnscheduleFromRunLoop (FW2::MONITOR_STREAM, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        FSEventStreamInvalidate(FW2::MONITOR_STREAM);
        FSEventStreamRelease(FW2::MONITOR_STREAM);
        FW2::MONITOR_STREAM = 0;
#else
        
#endif 
        //set flags
        FW2::MONITOR_PROCESS_SHOULD_TERMINATE = true;
        PA_YieldAbsolute();
				FW2::WATCH_PATHS.setSize(0);
				FW2::WATCH_PATHS_POSIX.setSize(0);
				FW2::CALLBACK_METHOD_ID = 0;
				FW2::CALLBACK_EVENT_PATHS.clear();
				FW2::CALLBACK_EVENT_FLAGS.clear();
				FW2::CALLBACK_EVENT_IDS.clear();
        //tell listener to die
        while(FW2::MONITOR_PROCESS_ID)
								{
            PA_YieldAbsolute();
            PA_UnfreezeProcess(FW2::MONITOR_PROCESS_ID);
        }
    }
} 

void listenerLoopExecute()
{
    FW2::MONITOR_PROCESS_SHOULD_TERMINATE = false;
    PA_UnfreezeProcess(FW2::MONITOR_PROCESS_ID);
}

void listenerLoopExecuteMethod()
{
	std::vector<CUTF16String>::iterator p = FW2::CALLBACK_EVENT_PATHS.begin();
	std::vector<NSTimeInterval>::iterator e = FW2::CALLBACK_EVENT_IDS.begin();
	std::vector<CUTF16String>::iterator f = FW2::CALLBACK_EVENT_FLAGS.begin();
	double ts = *e;
	CUTF16String event_path = *p;
	CUTF16String event_flag = *f;

	if(FW2::CALLBACK_METHOD_ID)
	{
		PA_Variable	params[3];
		params[0] = PA_CreateVariable(eVK_Unistring);
		params[1] = PA_CreateVariable(eVK_Real);
		params[2] = PA_CreateVariable(eVK_Unistring);
		PA_Unistring path = PA_CreateUnistring((PA_Unichar *)event_path.c_str());
		PA_Unistring flag = PA_CreateUnistring((PA_Unichar *)event_flag.c_str());

		PA_SetStringVariable(&params[0], &path);
		PA_SetRealVariable(&params[1], ts);
		PA_SetStringVariable(&params[2], &flag);
		
		FW2::CALLBACK_EVENT_IDS.erase(e);
		FW2::CALLBACK_EVENT_PATHS.erase(p);
		FW2::CALLBACK_EVENT_FLAGS.erase(f);
		
		PA_ExecuteMethodByID(FW2::CALLBACK_METHOD_ID, params, 3);
		//PA_DisposeUnistring(&path);//removed 15.11.20 (clear will dispose string too)
		PA_ClearVariable(&params[0]);//added 15.11.19
		PA_ClearVariable(&params[1]);
		PA_ClearVariable(&params[2]);
	}else{
		FW2::CALLBACK_EVENT_IDS.erase(e);
		FW2::CALLBACK_EVENT_PATHS.erase(p);
		FW2::CALLBACK_EVENT_FLAGS.erase(f);
	}
}

#pragma mark -

void PluginMain(PA_long32 selector, PA_PluginParameters params)
{
	try
	{
		PA_long32 pProcNum = selector;
		sLONG_PTR *pResult = (sLONG_PTR *)params->fResult;
		PackagePtr pParams = (PackagePtr)params->fParameters;

		CommandDispatcher(pProcNum, pResult, pParams); 
	}
	catch(...)
	{

	}
}

void CommandDispatcher (PA_long32 pProcNum, sLONG_PTR *pResult, PackagePtr pParams)
{
	switch(pProcNum)
	{
    
        case kInitPlugin :
        case kServerInitPlugin :            
            OnStartup();
            break;    

        case kCloseProcess :            
            OnCloseProcess();
            break;  
                
// --- Settings

		case 1 :
			FW_Set_watch_path(pResult, pParams);
			break;

		case 2 :
			FW_GET_WATCH_PATHS(pResult, pParams);
			break;

		case 3 :
			FW_Set_watch_method(pResult, pParams);
			break;

		case 4 :
			FW_Get_watch_method(pResult, pParams);
			break;

		case 5 :
			FW_Set_watch_paths(pResult, pParams);
			break;

	}
}

// ----------------------------------- Settings -----------------------------------


void FW_Set_watch_path(sLONG_PTR *pResult, PackagePtr pParams)
{
    C_TEXT Param1;
    C_LONGINT Param2;
		C_LONGINT returnValue;

		Param1.fromParamAtIndex(pParams, 1);
    Param2.fromParamAtIndex(pParams, 2);
    
    if(!Param1.getUTF16Length())
		{
			//empty string passed
			returnValue.setIntValue(1);
			if(FW2::WATCH_PATHS.getSize())
				PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopFinish, NULL);
    }else{
    
#if VERSIONMAC    
        
        BOOL isDirectory = false;    
        NSString *path = Param1.copyPath();
        NSString *pathHFS = Param1.copyUTF16String();
            
        if([[NSFileManager defaultManager]fileExistsAtPath:path isDirectory:&isDirectory]){
            
            if(isDirectory)
												{
                returnValue.setIntValue(1); 
                
                FW2::MONITOR_LATENCY = Param2.getIntValue();
                if(FW2::MONITOR_LATENCY < 1){
                    FW2::MONITOR_LATENCY = 1.0;
                }
                if(FW2::MONITOR_LATENCY > 60){
                    FW2::MONITOR_LATENCY = 60.0;
                }
            
								if(FW2::WATCH_PATHS.getSize())
									PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopFinish, NULL);
												
                FW2::WATCH_PATHS.setSize(0);
                FW2::WATCH_PATHS.appendUTF16String(@"");
                FW2::WATCH_PATHS.appendUTF16String(pathHFS);
                
                FW2::WATCH_PATHS_POSIX.setSize(0);
                FW2::WATCH_PATHS_POSIX.appendUTF16String(@"");
                FW2::WATCH_PATHS_POSIX.appendUTF16String(path);
																
								PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopStart, NULL);//restart always
													
            }else{
                returnValue.setIntValue(MONITOR_FOLDER_NOT_FOLDER_ERROR);
            }
            
        }else{
            returnValue.setIntValue(MONITOR_FOLDER_INVALID_PATH_ERROR);
        }
        
        [path release];
        [pathHFS release];
#else
        
#endif    
        
    }
    
	returnValue.setReturn(pResult);

}

void FW_GET_WATCH_PATHS(sLONG_PTR *pResult, PackagePtr pParams)
{
	FW2::WATCH_PATHS.toParamAtIndex(pParams, 1);
}

void FW_Set_watch_method(sLONG_PTR *pResult, PackagePtr pParams)
{
	C_TEXT Param1;
	C_LONGINT returnValue;

	Param1.fromParamAtIndex(pParams, 1);

    if(!Param1.getUTF16Length())
				{
								//empty string passed
        returnValue.setIntValue(1);
        FW2::CALLBACK_METHOD_ID = 0;
								FW2::WATCH_METHOD.setUTF16String((PA_Unichar *)"\0\0", 0);
								
    }else{  
        
        method_id_t methodId = PA_GetMethodID((PA_Unichar *)Param1.getUTF16StringPtr());
        
        if(methodId)
								{
            returnValue.setIntValue(1);
            
            if(methodId != FW2::CALLBACK_METHOD_ID)
												{
                FW2::WATCH_METHOD.setUTF16String(Param1.getUTF16StringPtr(), Param1.getUTF16Length());
                FW2::CALLBACK_METHOD_ID = methodId;
            }
        }else{
            returnValue.setIntValue(MONITOR_FOLDER_INVALID_METHOD_NAME_ERROR);
        }
    }
  
	returnValue.setReturn(pResult);
}

void FW_Get_watch_method(sLONG_PTR *pResult, PackagePtr pParams)
{
	FW2::WATCH_METHOD.setReturn(pResult);
}

void FW_Set_watch_paths(sLONG_PTR *pResult, PackagePtr pParams)
{
				ARRAY_TEXT Param1;
    C_LONGINT Param2;
				C_LONGINT returnValue;
    
				Param1.fromParamAtIndex(pParams, 1);
    Param2.fromParamAtIndex(pParams, 2);
				
				bool isEmpty = !Param1.getSize();
				
				if(Param1.getSize() == 1)
				{
								NSString *e1 = Param1.copyUTF16StringAtIndex(1);
								isEmpty = isEmpty || ![e1 length];
								[e1 release];
				}
				
    if(isEmpty)
				{
								//empty array passed
        returnValue.setIntValue(1);
				if(FW2::WATCH_PATHS.getSize())
					PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopFinish, NULL);

    }else{
    
#if VERSIONMAC    

        uint32_t i, length = Param1.getSize();
        
				if(FW2::WATCH_PATHS.getSize())
					PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopFinish, NULL);

        FW2::WATCH_PATHS.setSize(0);
        FW2::WATCH_PATHS_POSIX.setSize(0);
        FW2::WATCH_PATHS.appendUTF16String(@"");
        FW2::WATCH_PATHS_POSIX.appendUTF16String(@""); 
               
        for(i = 0; i < length; ++i){
        
            NSString *path = Param1.copyPathAtIndex(i);
            NSString *pathHFS = Param1.copyUTF16StringAtIndex(i);
            
            BOOL isDirectory = false;
            
            if([[NSFileManager defaultManager]fileExistsAtPath:path isDirectory:&isDirectory]){
                
                if(isDirectory)
																{
                    FW2::WATCH_PATHS.appendUTF16String(pathHFS);
                    FW2::WATCH_PATHS_POSIX.appendUTF16String(path);
                                        
                }else{
                    returnValue.setIntValue(MONITOR_FOLDER_NOT_FOLDER_ERROR);
                }
                
            }else{
                returnValue.setIntValue(MONITOR_FOLDER_INVALID_PATH_ERROR);
            }            
            
            [pathHFS release];            
            [path release];

        }
        
        if(FW2::WATCH_PATHS.getSize() > 1)
								{
            returnValue.setIntValue(1);
            FW2::MONITOR_LATENCY = Param2.getIntValue();
            if(FW2::MONITOR_LATENCY < 1)
												{
                FW2::MONITOR_LATENCY = 1.0;
            }
            if(FW2::MONITOR_LATENCY > 60)
												{
                FW2::MONITOR_LATENCY = 60.0;
            }
												
												PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopStart, NULL);
        }
#else
        
#endif    
        
    }
 
	returnValue.setReturn(pResult);
}

