/* iosstart.m: iOS-specific interface code for Glulx. (Objective C)
    Designed by Andrew Plotkin <erkyrath@eblong.com>
    http://eblong.com/zarf/glulx/index.html
*/

#import "TerpGlkViewController.h"
#import "TerpGlkDelegate.h"
#import "GlkStream.h"

#include "glk.h" /* This comes with the IosGlk library. */
#include "glulxe.h"
#include "iosstart.h"
#include "iosglk_startup.h" /* This comes with the IosGlk library. */

static void iosglk_game_start(void);

/* We don't load in the game file here. Instead, we set a hook which glk_main() will call back to do that. Why? Because of the annoying restartability of the VM under iosglk; we may finish glk_main() and then have to call it again.
 */
void iosglk_startup_code()
{
	set_library_start_hook(&iosglk_game_start);
	max_undo_level = 32; // allow 32 undo steps
}

/* This is the library_start_hook, which will be called every time glk_main() begins.
 */
static void iosglk_game_start()
{
	TerpGlkViewController *glkviewc = [TerpGlkViewController singleton];
	NSString *pathname = glkviewc.terpDelegate.gamePath;
	NSLog(@"iosglk_startup_code: game path is %@", pathname);
	
	gamefile = [[GlkStreamFile alloc] initWithMode:filemode_Read rock:1 unicode:NO textmode:NO dirname:@"." pathname:pathname];
	
	/* Now we have to check to see if it's a Blorb file. */
	int res;
	unsigned char buf[12];
	
	glk_stream_set_position(gamefile, 0, seekmode_Start);
	res = glk_get_buffer_stream(gamefile, (char *)buf, 12);
	if (!res) {
		init_err = "The data in this stand-alone game is too short to read.";
		return;
	}

	if (buf[0] == 'G' && buf[1] == 'l' && buf[2] == 'u' && buf[3] == 'l') {
		locate_gamefile(FALSE);
	}
	else if (buf[0] == 'F' && buf[1] == 'O' && buf[2] == 'R' && buf[3] == 'M'
			 && buf[8] == 'I' && buf[9] == 'F' && buf[10] == 'R' && buf[11] == 'S') {
		locate_gamefile(TRUE);
	}
	else {
		init_err = "This is neither a Glulx game file nor a Blorb file which contains one.";
	}
}

int iosglk_can_restart_cleanly()
{
 	return vm_exited_cleanly;
}

void iosglk_shut_down_process()
{
	/* Yes, we really do want to exit the app here. A fatal error has occurred at the interpreter level, so we can't restart it cleanly. The user has either hit a "goodbye" dialog button or the Home button; either way, it's time for suicide. */
	NSLog(@"iosglk_shut_down_process: goodbye!");
	exit(1);
}
