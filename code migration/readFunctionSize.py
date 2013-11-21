import os
fileptr = open("mRKv2.0/build/MRK.map","rb")
flag=0
for line in fileptr:
	tokens=line.split()
	if(flag==1):
		end=int(tokens[4])
		print tokens[4]
		flag=0
	if(len(tokens)> 1 and tokens[0]=='simple_function()'):
		flag=1
		start=int(tokens[4])
		print tokens[4]
fileptr.close()
functionsize = end-start
print functionsize


fin = open("senderNode.cpp","rb+")
fout = open("b.cpp", "wt")
for line in fin:
	tokens=line.split()
	if(len(tokens)>1 and tokens[1]=='functionSize'):
		token0=str(tokens[0])
		token1=str(tokens[1])
		token2=str(tokens[2])
		token3=str(functionsize)
		token4=str(tokens[4])
		line="\t"+"\t"+token0+" "+token1+" "+token2+" "+token3+" "+token4+"\n"
		fout.write(line)
	else :
		fout.write(line)
	#fout.write(line.replace('foo', 'bar') )
fin.close()
fout.close()
os.remove("senderNode.cpp")
os.rename("b.cpp","senderNode.cpp")