gif:
	convert -layers OptimizePlus -delay 3 -loop 0 girl1.png girl2.png girl.gif

help:
	$(info Usage: make [gif|help])

.PHONY: help gif
