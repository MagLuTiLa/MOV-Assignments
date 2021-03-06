// Template, major revision 6, update for INFOMOV
// IGAD/NHTV/UU - Jacco Bikker - 2006-2016

// Note:
// this version of the template uses SDL2 for all frame buffer interaction
// see: https://www.libsdl.org
// for glm (OpenGL mathematics) see http://glm.g-truc.net

#pragma warning (disable : 4530) // complaint about exception handler
#pragma warning (disable : 4273)
#pragma warning (disable : 4311) // pointer truncation from HANDLE to long

#include "template.h"

namespace Tmpl8 { 

float length( const float3& v ) { return sqrtf( v.x * v.x + v.y * v.y + v.z * v.z ); } 
float length( const float2& v ) { return sqrtf( v.x * v.x + v.y * v.y ); }
float3 normalize( const float3& v ) { float l = 1.0f / length( v ); return float3( v.x * l, v.y * l, v.z * l ); }
float2 normalize( const float2& v ) { float l = 1.0f / length( v ); return float2( v.x * l, v.y * l ); }
float dot( const float3& a, const float3& b ) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float dot( const float2& a, const float2& b ) { return a.x * b.x + a.y * b.y; }
float3 operator * ( const float3& v, const float& s ) { return float3( v.x * s, v.y * s, v.z * s ); }
float2 operator * ( float2& v, float& s ) { return float2( v.x * s, v.y * s ); }

void NotifyUser( char* s )
{
	HWND hApp = FindWindow( NULL, "Template" );
	MessageBox( hApp, s, "ERROR", MB_OK );
	exit( 0 );
}
}

static int SCRPITCH = 0;
int ACTWIDTH, ACTHEIGHT;
static bool FULLSCREEN = false, firstframe = true;

Surface* surface = 0;
Game* game = 0;
float lastftime = 0;
LARGE_INTEGER lasttime, ticksPS;
double Timer::inv_freq = 1;

float GetTime()
{
	LARGE_INTEGER freq, value;
	QueryPerformanceCounter( &value );
	QueryPerformanceFrequency( &freq );
	return (float)((double)value.QuadPart / (double)freq.QuadPart);
}

static LONGLONG startTime = 0;
void StartTimer()
{
	LARGE_INTEGER value;
	QueryPerformanceCounter( &value );
	startTime = value.QuadPart;
}

float GetDuration()
{
	LARGE_INTEGER freq, value;
	LONGLONG elapsed;
	QueryPerformanceCounter( &value );
	QueryPerformanceFrequency( &freq );
	elapsed = value.QuadPart - startTime;
	return (float)((double)elapsed / (double)freq.QuadPart);
}

void redirectIO()
{
	static const WORD MAX_CONSOLE_LINES = 500;
	int hConHandle;
	long lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE *fp;
	AllocConsole();
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),
	&coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE),
	coninfo.dwSize);
	lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stdout = *fp;
	setvbuf( stdout, NULL, _IONBF, 0 );
	lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "r" );
	*stdin = *fp;
	setvbuf( stdin, NULL, _IONBF, 0 );
	lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stderr = *fp;
	setvbuf( stderr, NULL, _IONBF, 0 );
	ios::sync_with_stdio();
	freopen( "CON", "w", stdout );
	freopen( "CON", "w", stderr );
}

int main( int argc, char **argv ) 
{  
	redirectIO();
	printf( "application started.\n" );
	SDL_Init( SDL_INIT_VIDEO );
	surface = new Surface( SCRWIDTH, SCRHEIGHT );
	surface->Clear( 0 );
	surface->InitCharset();
	SDL_Window* window = SDL_CreateWindow( "Template", 100, 100, SCRWIDTH, SCRHEIGHT, SDL_WINDOW_SHOWN );
	SDL_Renderer* renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
	SDL_Texture* frameBuffer = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCRWIDTH, SCRHEIGHT );
	int exitapp = 0;
	game = new Game();
	game->SetTarget( surface );
	while (!exitapp) 
	{
		void* target = 0;
		int pitch;
		SDL_LockTexture( frameBuffer, NULL, &target, &pitch );
		if (pitch == (surface->GetWidth() * 4))
		{
			memcpy( target, surface->GetBuffer(), SCRWIDTH * SCRHEIGHT * 4 );
		}
		else
		{
			unsigned char* t = (unsigned char*)target;
			for( int i = 0; i < SCRHEIGHT; i++ )
			{
				memcpy( t, surface->GetBuffer() + i * SCRWIDTH, SCRWIDTH * 4 );
				t += pitch;
			}
		}
		SDL_UnlockTexture( frameBuffer );
		SDL_RenderCopy( renderer, frameBuffer, NULL, NULL );
		SDL_RenderPresent( renderer );
		if (firstframe)
		{
			game->Init(false);
			firstframe = false;
		}
		// calculate frame time and pass it to game->Tick
		StartTimer();
		game->Tick( lastftime );
		lastftime = GetDuration();
		// event loop
		SDL_Event event;
		while (SDL_PollEvent( &event )) 
		{
			switch (event.type)
			{
			case SDL_QUIT:
				exitapp = 1;
				break;
			 case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) 
				{
					exitapp = 1;
					// find other keys here: http://sdl.beuc.net/sdl.wiki/SDLKey
				}
				game->KeyDown( event.key.keysym.scancode );
				break;
			case SDL_KEYUP:
				game->KeyUp( event.key.keysym.scancode );
				break; 
			case SDL_MOUSEMOTION:
				game->MouseMove( event.motion.x, event.motion.y );
				break;
			case SDL_MOUSEBUTTONUP:
				game->MouseButton( false );
				break;
			case SDL_MOUSEBUTTONDOWN:
				game->MouseButton( true );
				break;
			default:
				break;
			}
		}
	}
	SDL_Quit();
	return 1;
}