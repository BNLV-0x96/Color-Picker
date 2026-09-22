//This is a work in progress.
//Currently, the program runs on console application with a graphical window that can be interacted with.
//Input must be done in the console. It is planned to turn this project into an interactive GUI window only.

#include <stdio.h>
#include <Windows.h>
#include <wingdi.h>
#include <Windowsx.h>
#define MK_LBUTTONDOWN 0x0201

static bool running;

struct win32_offscreen_buffer 
{
	BITMAPINFO bitmapInfo;
	void* memory;
	int width;
	int height;
	int pitch;
	int bytesPerPixel;
};

struct win32_offscreen_buffer windowsBackbuffer;

struct program_offscreen_buffer 
{
	void* memory;
	int width;
	int height;
	int pitch;
	int bytesPerPixel;
};

struct ClickEvent 
{
	int xPos;
	int yPos;
	bool isDown;
};

struct Color
{
	int r;
	int g;
	int b;
};


ClickEvent clickEvent;

class Button 
{
	public: 
		int minX, minY, maxX, maxY, width, height;

	Button(int x, int y, int w, int h)
	{
		minX = x;
		minY = y;
		width = w;
		height = h;

		maxX = minX + w;
		maxY = minY + h;
	}

	//Checks if the cursor is on the button when any click event below responds
	bool CursorOnButton(ClickEvent click) 
	{
		if (click.xPos > minX && click.yPos > minY && click.xPos < maxX && click.yPos < maxY && click.isDown)
		{
			return true;
		}

		return false;
	}

	//When the cursor is hovering on the button
	bool OnHover(ClickEvent click) 
	{
		if (CursorOnButton(click))
		{
			return true;
		}

		return false;
	}

	//When the button is clicked
	bool OnClick(ClickEvent click)
	{
		if (CursorOnButton(click)) 
		{
			return true;
		}
		
		return false;
	}

	//When the button is clicked and released
	bool OnClickAndRelease(ClickEvent click)
	{
		if (CursorOnButton(click))
		{
			return true;
		}

		return false;
	}
};

//Windows messages
LRESULT Wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	HDC hdc;
	LRESULT result = 0;

	switch (message) 
	{
	case MK_LBUTTONDOWN:

		clickEvent.xPos = GET_X_LPARAM(lParam);
		clickEvent.yPos = GET_Y_LPARAM(lParam);
		clickEvent.isDown = true;
		break;

	case WM_LBUTTONUP:
		clickEvent.xPos = GET_X_LPARAM(lParam);
		clickEvent.yPos = GET_Y_LPARAM(lParam);
		clickEvent.isDown = false;
		break;

	default:
		result = DefWindowProc(hwnd, message, wParam, lParam);
	}

	return result;
}

//Used for graphical window
static void ResizeDIBSection(struct win32_offscreen_buffer* buffer, int width, int height) 
{
	if (buffer->memory) 
	{
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}

	buffer->width = width;
	buffer->height = height;
	buffer->bytesPerPixel = 4;

	buffer->bitmapInfo.bmiHeader.biSize = sizeof(buffer->bitmapInfo.bmiHeader);
	buffer->bitmapInfo.bmiHeader.biWidth = buffer->width;
	buffer->bitmapInfo.bmiHeader.biHeight = -buffer->height;
	buffer->bitmapInfo.bmiHeader.biPlanes = 1;
	buffer->bitmapInfo.bmiHeader.biBitCount = 32;
	buffer->bitmapInfo.bmiHeader.biCompression = BI_RGB;

	int bitmapMemorySize = (buffer->width * buffer->height) * buffer->bytesPerPixel;

	buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	buffer->pitch = buffer->width * buffer->bytesPerPixel;
}

//Used for graphical window
static void Win32UpdateWindow(HDC hdc, struct win32_offscreen_buffer buffer) 
{
	StretchDIBits(hdc, 
		0, 
		0, 
		buffer.width, 
		buffer.height, 
		0, 
		0, 
		buffer.width, 
		buffer.height, 
		buffer.memory, 
		&buffer.bitmapInfo, 
		DIB_RGB_COLORS, 
		SRCCOPY);
	
}

//Windows messages
static void ProcessPendingMessage()
{
	MSG msg;


	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			running = false;
		}

		switch (msg.message)
		{
			default:
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				break;
		}
	}
}

//Renders the window backbuffer with the chosen color
static void RenderBackbuffer(program_offscreen_buffer* buffer, Color color) 
{
	//Sets the minimum offset in X and Y coordinates
	int posX = 0;
	int posY = 0;
	
	//Sets the space for the buffer to render
	unsigned char* row = (unsigned char*)buffer->memory;

	//Rendering go brrr
	for (int y = 0; y < buffer->height; y++) 
	{
		unsigned char* pixel = row;

		for (int x = 0; x < buffer->width; x++) 
		{
			*pixel = color.b;
			++pixel;
			*pixel = color.g;
			++pixel;
			*pixel = color.r;
			++pixel;
			*pixel = 0;
			++pixel;

		}
	
		row += buffer->pitch;
	}
}

static void RenderRectangle(program_offscreen_buffer* buffer, Color color, int posX, int posY, int width, int height)
{
	//Sets the minimum and maximum offset in X and Y coordinates
	int minX = posX;
	int minY = posY;
	int maxX = minX + width;
	int maxY = minY + height;

	//Sets where the rectangle will appear in the buffer
	unsigned char* row = (unsigned char*)buffer->memory + minX * buffer->bytesPerPixel + minY * buffer->pitch;

	//Rendering go brrr
	for (int y = minY; y < maxY; y++)
	{
		unsigned char* pixel = row;

		for (int x = minX; x < maxX; x++)
		{
			*pixel = color.b;
			++pixel;
			*pixel = color.g;
			++pixel;
			*pixel = color.r;
			++pixel;
			*pixel = 0;
			++pixel;

		}

		row += buffer->pitch;
	}

}

//Sets RGB color in one structure
static Color SetColor(int r, int g, int b)
{
	Color color;

	color.r = r;
	color.g = g;
	color.b = b;

	return color;
}

//int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
int main()
{

	WNDCLASS windowClass = {};
	
	win32_offscreen_buffer buffer;
	Color color = {};
	Button button = Button(0, 0, 25, 25);

	windowClass.lpszClassName = "ClassNameWindow";
	windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = Wndproc;
	//windowClass.hInstance = hInstance;

	HDC hdc;
	HWND hwnd; 
	MSG msg;
	Color colorSet = {};

	if (RegisterClassA(&windowClass)) 
	{
		//Creates window
		hwnd = CreateWindowEx(0, 
			windowClass.lpszClassName, 
			"Color Picker", 
			WS_POPUPWINDOW | WS_VISIBLE, 
			CW_USEDEFAULT, CW_USEDEFAULT, 200, 200, 
			0, 0, 0, 0);

		if (hwnd) 
		{
			hdc = GetDC(hwnd);

			ResizeDIBSection(&windowsBackbuffer, 200, 200);

			running = true;

			printf("[Color Picker Window]\nTo change the color, click the button in the top corner of the preview window.\n");

			//Loops the program while running.
			while(running) 
			{
				ProcessPendingMessage();

				program_offscreen_buffer buffer;

				//Sets program buffer from the window's buffer for rendering
				buffer.memory = windowsBackbuffer.memory;
				buffer.width = windowsBackbuffer.width;
				buffer.height = windowsBackbuffer.height;
				buffer.pitch = windowsBackbuffer.pitch;
				buffer.bytesPerPixel = windowsBackbuffer.bytesPerPixel;

				//Window render
				RenderBackbuffer(&buffer, SetColor(colorSet.r, colorSet.g, colorSet.b));

				//Button render
				RenderRectangle(&buffer, SetColor(122, 122, 122), button.minX, button.minY, button.width, button.height);
				RenderRectangle(&buffer, SetColor(255, 255, 255), button.minX, button.minY, button.width - 2, button.height - 2);
				RenderRectangle(&buffer, SetColor(200, 200, 200), button.minX + 2, button.minY + 2, button.width - 4, button.height - 4);
				
				//If the button is clicked, the console prompts the user to enter three colors in integer.
				if (button.OnClick(clickEvent)) 
				{
					printf("\nPlease enter three colors (r, g, b) in range of 0-255 to set.\n");
					
					scanf("%d %d %d", &colorSet.r, &colorSet.g, &colorSet.b);

					printf("Color set to{%d, %d, %d}\n", colorSet.r, colorSet.g, colorSet.b);
				}

				Win32UpdateWindow(hdc, windowsBackbuffer);

				ReleaseDC(hwnd, hdc);
			}
		}
		else 
		{
			MessageBoxA(hwnd, "Error with handle", "Error", MB_ICONERROR);
		}

	}
	else 
	{
		printf("Window class registration failed.\n");
	}

	return 0;
}