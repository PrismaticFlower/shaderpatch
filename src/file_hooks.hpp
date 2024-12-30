#include <Windows.h>

extern "C" HANDLE WINAPI CreateFileA_hook(LPCSTR file_name,
                                          DWORD desired_access, DWORD share_mode,
                                          LPSECURITY_ATTRIBUTES security_attributes,
                                          DWORD creation_disposition,
                                          DWORD flags_and_attributes,
                                          HANDLE template_file);