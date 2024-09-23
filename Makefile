CC = gcc
MAIN = project
FUNC = chessfunc
GFX = gfx
EXEC = project

$(EXEC): $(MAIN).o $(FUNC).o $(GFX).o
	$(CC) $(MAIN).o $(FUNC).o $(GFX).o -lX11 -o $(EXEC)

$(FUNC).o: $(FUNC).c $(FUNC).h
	$(CC) -c $(FUNC).c -o $(FUNC).o

$(MAIN).o: $(MAIN).c $(FUNC).h
	$(CC) -c $(MAIN).c -o $(MAIN).o


clean:
	rm $(MAIN).o $(FUNC).o
	rm $(EXEC)

