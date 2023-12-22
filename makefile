run: 7392.cpp
	g++ -o main $< -lpthread > output.txt
	./main >> output.txt


clean:
	rm -f main output.txt
