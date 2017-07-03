#!/usr/bin/python3
# Autor: Ondřej Šlampa
# Email: xslamp01@stud.fit.vutbr.cz
# Project: Program, který provede testování pomocí testovacích případů.

import os
import subprocess
import collections

NGEN=50
POP_SIZE=10
PRINT_EACH=False
REPEAT=10

crossovers=["pmx","order"]
decoders=["cover","cover+","contribution","combined"]
number_of_cases=int(len(os.listdir("data"))/2)

feasible_sums=collections.defaultdict(int)
fitness_sums=collections.defaultdict(int)
time_sums=collections.defaultdict(int)

for case in range(0,number_of_cases):
	for decoder in decoders:
		for crossover in crossovers:
			iden=(decoder, crossover)
			fitness_sum=0
			feasible_sum=0
			time_sum=0.0
			for i in range(0,REPEAT):
				(stdout,stderr)=subprocess.Popen(["time","-p","./main","-t","-d"+decoder,"-x"+crossover,"-ndata/"+str(case)+"_nurses.txt","-rdata/"+str(case)+"_requirements.txt","ngen",str(NGEN),"popsize",str(POP_SIZE)],stdout=subprocess.PIPE,stderr=subprocess.PIPE).communicate()
				[fitness,feasible]=stdout.split()
				fitness_sum+=int(fitness)
				feasible_sum+=int(feasible)
				time_sum+=float(stderr.split()[1])
			feasible_sums[iden]+=100.0*feasible_sum/REPEAT
			fitness_sums[iden]+=fitness_sum
			time_sums[iden]+=time_sum
			if PRINT_EACH:
				print("{0:2} {1:12} {5:5} {2:4} {3:4} {4:5}".format(case,decoder,int(fitness_sum/REPEAT),int(100.0*feasible_sum/REPEAT),time_sum/REPEAT,crossover))

print("{0:12} {4:7} {1:7} {2:10} {3:7}".format("decoder","fitness","feasible","time","cross"))
n=REPEAT*number_of_cases
for decoder in decoders:
	for crossover in crossovers:
		iden=(decoder, crossover)
		print("{0:12} {4:7} {1:7} {2:7} {3:7}".format(decoder,int(fitness_sums[iden]/n),int(feasible_sums[iden]/number_of_cases),time_sums[iden]/n,crossover))

