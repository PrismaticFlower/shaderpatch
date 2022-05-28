#pragma once

#include <Windows.h>
#include <d3d11.h>

void array_maker_init(ID3D11Device& device, HWND window);

void array_maker(const float display_scale);

void array_maker_add_drop_files(HDROP files);
