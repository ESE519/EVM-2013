import os
functions = []
functionSizes = []
#get the list of functions from the header file
def get_functions():
	count=0
	fileptr = open("funcs.h",'rb+')
	for func in fileptr:
        	if(count==1):
			functions = func.split();
		        #print functions
		if(count>2):
			break
		else:
			count=count+1
	fileptr.close()
	return functions

	
# reading the function sizes from the map file
def get_functionsize(function):
	flag=0
	end=0
	start=0
	fileptr = open("mRKv2.0/build/MRK.map","rb")
	for line in fileptr:
		tokens=line.split()
		if(flag==1):
			end=int(tokens[4])
			#print tokens[4]
			flag=0
		if(len(tokens)> 1 and tokens[0]== function):
			flag=1
			start=int(tokens[4])
			#print tokens[4]
	fileptr.close()
	functionsize = end-start
	return functionsize

#writing the function size to the header file
def write_functionSizes():
	fin = open("funcs.h","rb+")
	fout = open("b.h", "wt")
        index=0
	flagstart=0
	flagend=0
	print functions
	for line in fin:
		tokens=line.split()
		
		if(flagstart==1 and flagend==0 and index<len(functionSizes)):
			token0=str(tokens[0])
			token1=str(functionSizes[index])
			print token1
			if(index==len(functions)-1):
				line= token0+" "+token1+"\n"	
  			else:				
				line= token0+" "+token1+",\n"
			fout.write(line)
			#print functions[index]
			index=index+1
		else :
			fout.write(line)
		if(len(tokens)>0 and tokens[0]=="//start"):
			flagstart=1
		if(len(tokens)>0 and tokens[0]=="//end"):
			flagend=1
	fin.close()
	fout.close()
	os.remove("funcs.h")
	os.rename("b.h","funcs.h")


functions=get_functions()
for function in functions:
	functionSizes.append(get_functionsize(function))
write_functionSizes()





	
	

