#!/usr/bin/python3
# Autor: Ondřej Šlampa
# Email: xslamp01@stud.fit.vutbr.cz
# Project: Program, který vygeneruje testovací případy.

from random import randint,gauss
import os,shutil

NUMBER_OF_CASES=100
SIGMA=0.3

if os.path.exists("data"):
	shutil.rmtree("data")
os.mkdir("data")

for case in range(0,NUMBER_OF_CASES):
	grades=[0,0,0]
	n=randint(10,30)
	
	with open("data/"+str(case)+"_nurses.txt","w") as nurses_file:
		for nurse in range(0,n):
			name="n"+str(nurse)
			grade=randint(0,2)
			grades[grade]+=1
			preference=[]
			for _ in range(0,14):
				preference.append(str(randint(0,10)))
			preference=",".join(preference)
			out=",".join([name,str(grade),preference])
			print(out,file=nurses_file)
	
	grades[1]+=grades[2]
	grades[0]+=grades[1]
	
	with open("data/"+str(case)+"_requirements.txt","w") as requirements_file:
		day=[]
		night=[]
		for i in range(0,len(grades)):
			g=(grades[i]*4)/14
			day.append(2*g*3/4)
			night.append(2*g/4)
			
		for _ in range(0,7):
			a=[]
			for i in range(0,len(day)):
				a.append(str(round(gauss(day[i],SIGMA))))
			out=",".join(a)
			print(out,file=requirements_file)
		for _ in range(7,14):
			a=[]
			for i in range(0,len(night)):
				a.append(str(round(gauss(night[i],SIGMA))))
			out=",".join(a)
			print(out,file=requirements_file)
		
				
