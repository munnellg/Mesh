#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#define FPS  30.0f
#define MSPF (1000.0f / FPS)

#define DEFAULT_WINDOW_WIDTH  450
#define DEFAULT_WINDOW_HEIGHT 450

struct vertex {
    float x, y, z;
};

struct mesh {
    struct vertex *v;
    int size;
};

struct state {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *texture;

    int win_height;
    int win_width;
    int win_flags;

    int cur_mesh;
    int num_meshes;
    struct mesh *meshes;
    char *meshfile;

    float angle;

    Uint32 time;
    int quit;
} state;

static int
load_meshes ( const char *fname )
{
    FILE *f;
    
    f = fopen( fname, "r" );
    if (!f) { return 0; }

    fscanf( f, "%d", &state.num_meshes );
    state.meshes = malloc( sizeof(struct mesh) * state.num_meshes );
    if ( !state.meshes ) { return 0; }
    memset( state.meshes, 0, sizeof( struct mesh ) * state.num_meshes );

    for ( int i = 0; i < state.num_meshes; i++ ) {
        fscanf( f, "%d", &state.meshes[i].size );
        state.meshes[i].v = malloc( 
            sizeof( struct vertex ) * state.meshes[i].size
        );
        
        if ( !state.meshes[i].v ) { return 0; };

        for ( int j = 0; j < state.meshes[i].size; j++ ) {
            fscanf( 
                f, "%f %f %f", 
                &state.meshes[i].v[j].x, 
                &state.meshes[i].v[j].y, 
                &state.meshes[i].v[j].z 
            );
        }
    }

    fclose(f);

    return 1;
}

static void
quit ( void )
{
    if ( state.meshes ) {
        for ( int i = 0; i < state.num_meshes; i++ ) {
            if ( state.meshes[i].v ) {
                free( state.meshes[i].v );
            }
        }
        free(state.meshes);
    }
    if ( state.texture )  { SDL_DestroyTexture( state.texture ); }
    if ( state.renderer ) { SDL_DestroyRenderer( state.renderer ); }
    if ( state.window )   { SDL_DestroyWindow( state.window ); }
    SDL_Quit();
}

static void
die ( const char *msg, ... )
{
    va_list args;
    
    va_start( args, msg );
    fprintf( stderr, "ERROR: " );
    vfprintf( stderr, msg, args );
    fprintf( stderr, "\n" );
    va_end( args );

    quit();
    exit(EXIT_FAILURE);
}

static void
usage ( const char *progname )
{
    fprintf( stderr, "usage: %s [options] FILE\n", progname );
    fprintf( stderr, "\n" );
    fprintf( stderr, "options:\n" );
    fprintf( stderr, "\t-W\tset window width\n" );
    fprintf( stderr, "\t-H\tset window height\n" );
    fprintf( stderr, "\t-h\tprint this help message\n" );
    exit(EXIT_SUCCESS);
}

static void
parse_args ( int argc, char *argv[] )
{
    memset( &state, 0, sizeof(struct state) );

    state.win_width  = DEFAULT_WINDOW_WIDTH;
    state.win_height = DEFAULT_WINDOW_HEIGHT;

    for ( int i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {
            if ( strcmp( "-h", argv[i] ) == 0 ) {
                usage( argv[0] );
            } else if ( strcmp( "-f", argv[i] ) == 0 ) {
                state.win_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;  
            } else if ( strcmp( "-H", argv[i] ) == 0 ) {
            
                if ( ++i >= argc ) { 
                    die( "\"-H\" expects a numerical argument" ); 
                }
                
                int tmp = atoi( argv[i] );
                if ( tmp <= 0 ) {
                    die ( 
                        "Window height must be positive "
                        "and greater than zero" 
                    );
                }

                state.win_height = tmp;
            } else if ( strcmp ( "-W", argv[i] ) == 0 ) {
                if ( ++i >= argc ) { 
                    die( "\"-W\" expects a numerical argument" ); 
                }
            
                int tmp = atoi( argv[i] );
                if ( tmp <= 0 ) {
                    die ( 
                        "Window width must be positive "
                        "and greater than zero" 
                    );
                }   

                state.win_width = tmp;
            } else {
                die( "Invalid argument \"%s\"", argv[i] );
            }
        } else {
            state.meshfile = argv[i];
        }
    }

    if ( state.meshfile == NULL ) {
        die("program expects mesh file as input");
    }
}

static void
init ( void ) 
{
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
        die( "%s", SDL_GetError() );
    }

    int stat = SDL_CreateWindowAndRenderer( 
            state.win_width, 
            state.win_height, 
            state.win_flags, 
            &state.window, 
            &state.renderer 
    );

    if ( stat < 0 ) {
        die ( "%s", SDL_GetError() );
    }

    state.texture = SDL_CreateTexture ( 
            state.renderer, 
            SDL_PIXELFORMAT_ARGB8888, 
            SDL_TEXTUREACCESS_STREAMING, 
            state.win_width, 
            state.win_height 
    );

    if ( state.texture == NULL ) {
        die( "%s", SDL_GetError() );
    }

    if ( !load_meshes( state.meshfile ) ) {
        die( "failed to load mesh from \"%s\"", state.meshfile );
    }

}

static void
update ( void ) 
{
    Uint32 now = SDL_GetTicks();
    Sint32 diff = MSPF - now + state.time;    
    SDL_Delay( (diff < 0) ? 0 : diff );
    state.angle += 0.01;
    state.cur_mesh = ( state.cur_mesh + 1 ) % state.num_meshes;
    state.time = SDL_GetTicks();
}

static void
events ( void ) 
{
    SDL_Event e;
    while ( SDL_PollEvent(&e) ) {
        switch (e.type) {
        case SDL_QUIT:
            state.quit = 1;
            break;
        case SDL_KEYDOWN:
            switch ( e.key.keysym.sym ) {
            case SDLK_q:
                state.quit = 1;
            }
            break;
        }
    }
}

static void
render ( void ) 
{
    int pitch;
    void *texbuf;
    Uint32 *pixels;

    SDL_LockTexture( state.texture, NULL, &texbuf, &pitch );

    pixels = texbuf;
    memset( pixels, 0, sizeof(Uint32) * state.win_width * state.win_height );
    
    float vsin = sin(state.angle);
    float vcos = cos(state.angle);
    for ( int i = 0; i < state.meshes[state.cur_mesh].size; i++ ) {
        struct vertex v = state.meshes[state.cur_mesh].v[i];
        struct vertex vp;
        vp.x = vcos * v.x - vsin * v.z ;
        vp.y = v.y - 30;
        vp.z = vcos * v.z  + vsin * v.x;

        vp.x *= 1.5f;
        vp.y *= -1.5f;

        vp.x += ( state.win_width / 2 );
        vp.y += ( state.win_height / 2 + 50 );

        if ( vp.x >= 0 && vp.y >= 0 ) {
            pixels[ (int) vp.x + (int) vp.y * state.win_width ] = 0xFFFFFF;
        }
    } 

    SDL_UnlockTexture( state.texture );

    SDL_RenderClear( state.renderer );
    SDL_RenderCopy( state.renderer, state.texture, NULL, NULL );
    SDL_RenderPresent( state.renderer );
}

static void
loop ( void )
{
    while ( !state.quit ) {
        update();
        events();
        render();
    }
}

int
main ( int argc, char *argv[] )
{
    parse_args( argc, argv );

    init();
    loop();
    quit();

    return EXIT_SUCCESS;
}
