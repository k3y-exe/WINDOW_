#include "window.h"
#include <stdio.h>
#include <unistd.h>

int main(void)
{
	WINDOW_Window window;
	int result;

	result = WINDOW_WindowCreate(&window, "heh freakyzoid", 800, 600);
	if (!result) {
		printf("Failed to create window\n");
		return 1;
	}

	while (!WINDOW_WindowShouldClose(&window)) {
		WINDOW_WindowPoll(&window);

		usleep(16000);
	}

	WINDOW_WindowDestroy(&window);
	return 0;
}

