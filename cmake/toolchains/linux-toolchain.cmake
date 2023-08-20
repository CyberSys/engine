include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckIncludeFiles)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads)

if (CMAKE_COMPILER_IS_GNUCXX)
	check_function_exists(__atomic_fetch_add_4 HAVE___ATOMIC_FETCH_ADD_4)
	if (NOT HAVE___ATOMIC_FETCH_ADD_4)
		check_library_exists(atomic __atomic_fetch_add_4 "" HAVE_LIBATOMIC)
		if (HAVE_LIBATOMIC)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -latomic")
		endif()
	endif()
endif()

check_library_exists(m pow "" HAVE_LIBM)
if(HAVE_LIBM)
	set(CMAKE_REQUIRED_LIBRARIES m)
	foreach(_FN
		atan atan2 ceil copysign cos cosf fabs floor log pow scalbn sin
		sinf sqrt sqrtf tan tanf acos asin)
		string(TOUPPER ${_FN} _UPPER)
		set(_HAVEVAR "HAVE_${_UPPER}")
		check_function_exists("${_FN}" ${_HAVEVAR})
	endforeach()
	set(CMAKE_REQUIRED_LIBRARIES)
	set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} -lm")
endif()

#set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} ${CMAKE_DL_LIBS}")

check_include_files("syslog.h" HAVE_SYSLOG_H)
check_include_files("execinfo.h" HAVE_EXECINFO_H)
check_include_files("sys/resource.h" HAVE_SYS_RESOURCE_H)
check_include_files("sys/time.h" HAVE_SYS_TIME_H)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -D_GNU_SOURCE -D_BSD_SOURCE -D_DEFAULT_SOURCE -D_XOPEN_SOURCE")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -D_FORTIFY_SOURCE=2")
foreach(_FLAG
	-Wreturn-type -Wwrite-strings -Wno-unused-parameter -fdiagnostics-color=auto -ftime-trace)
	string(REPLACE "=" "_" _NAME ${_FLAG})
	string(REPLACE "-" "_" _NAME ${_NAME})
	check_c_compiler_flag(${_FLAG} HAVE_FLAG${_NAME})
	if (HAVE_FLAG${_NAME})
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_FLAG}")
	endif()
endforeach()
if (CMAKE_USE_PTHREADS_INIT)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
endif()

foreach(_FLAG
	-fexpensive-optimizations -fomit-frame-pointer)
	string(REPLACE "=" "_" _NAME ${_FLAG})
	string(REPLACE "-" "_" _NAME ${_NAME})
	check_c_compiler_flag(${_FLAG} HAVE_FLAG${_NAME})
	if (HAVE_FLAG${_NAME})
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${_FLAG}")
	endif()
endforeach()

foreach(_FLAG
	-fno-omit-frame-pointer)
	string(REPLACE "=" "_" _NAME ${_FLAG})
	string(REPLACE "-" "_" _NAME ${_NAME})
	check_c_compiler_flag(${_FLAG} HAVE_FLAG${_NAME})
	if (HAVE_FLAG${_NAME})
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${_FLAG}")
	endif()
endforeach()
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_C_FLAGS_RELEASE}")
if (USE_SANITIZERS)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize=undefined")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_C_FLAGS_DEBUG} -fstack-protector-strong -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer ${SANITIZE_FLAGS}")
