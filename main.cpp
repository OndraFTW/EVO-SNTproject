/*
	Autor:Ondřej Šlampa
	Email:xslamp01@stud.fit.vutbr.cz
	Projekt:Nurse rostering problem
	Popis:Program, který vyřeší nurse rostering problem pomocí genetického algoritmu.
*/

//TODO globální proměnné

#include<iostream>
#include<fstream>
#include<limits>
#include<vector>
#include<string>
#include<bitset>
#include<ga/ga.h>
#include<getopt.h>
#include<stdexcept>

/*
	Definice konstant.
*/
#define DAYS_IN_WEEK 7
#define MAX_DAY_SHIFTS 5
#define MAX_NIGHT_SHIFTS 4
#define NUMBER_OF_GRADES 3
#define NUMBER_OF_SHIFTS 14
#define NUMBER_OF_PATTERNS 56
#define WEIGHT 20
#define PREFERENCE_WEIGHT 1
#define DEFAULT_DUMMY_PATTERN 0
#define DEFAULT_CROSSOVER PMX
#define DEFAULT_NURSES_FILE "nurses.txt"
#define DEFAULT_REQUIREMENTS_FILE "requirements.txt"

using namespace std;

/*
	Typ dekodéru.
*/
enum Decoder{
	DUMMY,
	COVER,
	COVER_PLUS,
	CONTRIBUTION,
	COMBINED
};

/*
	Typ křížení.
*/
enum Crossover{
	PMX,
	ORDER
};

/*
	Zpracované parametry příkazové řádku.
*/
struct Args{
	bool error;
	string nurses_file;
	string requirements_file;
	bool help;
	bool testing;
	Crossover crossover;
};

//váhy nezaplňených míst podle stupňů kvalifikace
const int UNCOVERED_WEIGHT[]={1,2,8};

vector<bitset<NUMBER_OF_GRADES>> q;
vector<int*> p;
vector<int*> R;
vector<bitset<NUMBER_OF_SHIFTS>> a;
vector<string> names;
int number_of_nurses=0;
Decoder decoder=COMBINED;
int dummy_pattern=DEFAULT_DUMMY_PATTERN;

/*
	Vytvoří seznam všech možných vzorů směn.
*/
void create_patterns(){
	a=vector<bitset<NUMBER_OF_SHIFTS>>();
	for(unsigned int i=0;i<128;i++){
		bitset<NUMBER_OF_SHIFTS>s(i);
		int c=s.count();
		if(c==MAX_NIGHT_SHIFTS){
			s<<=NUMBER_OF_SHIFTS/2;
			a.push_back(s);
		}
		if(c==MAX_DAY_SHIFTS){
			a.push_back(s);
		}
	}
}

/*
	Uvolní seznam všech možných vzorů směn.
*/
void free_patterns(){
	a.clear();
}

/*
	Načte požadavky na počet sestřiček.
*/
void load_requirements(string f){
	R=vector<int*>();
	ifstream input_file;
    input_file.open(f, ios::in);
    for(int i=0;i<NUMBER_OF_SHIFTS;i++){
    	string s;
    	int* t=new int[NUMBER_OF_GRADES];
    	for(int j=0;j<NUMBER_OF_GRADES-1;j++){
			getline(input_file,s,',');
			t[j]=stoi(s);
		}
		getline(input_file,s);
		t[NUMBER_OF_GRADES-1]=stoi(s);
		R.push_back(t);
    }
}

/*
	Uvolní požadavky na počet sestřiček.
*/
void free_requirements(){
	for(vector<int*>::iterator i=R.begin();i<R.end();i++){
		delete[] *i;
	}
}

/*
	Načte data o sestřičkách.
	f umístění souboru s saty o sestřičkách
*/
void load_nurses(string f){
	int* aa=new int[NUMBER_OF_SHIFTS];
	p=vector<int*>();
	q=vector<bitset<NUMBER_OF_GRADES>>();
	ifstream input_file;
    input_file.open(f, ios::in);
    
    while(input_file.good()){
    	string s;
    	
    	//načtení jména
    	getline(input_file,s,',');
    	names.push_back(s);
    	
		//načtení stupně kvalifikace
    	int g=0;
    	bitset<NUMBER_OF_GRADES> gg=bitset<NUMBER_OF_GRADES>(0);
    	getline(input_file,s,',');
    	try{
    		g=stoi(s);
    	}
    	catch(const invalid_argument& e){
    		names.pop_back();
    		break;
    	}
    	//uložení stupně kvalifikace do struktury q
    	for(int i=0;i<NUMBER_OF_GRADES;i++){
    		gg[i]=g>=i?1:0;
    	}
    	q.push_back(gg);
    	
    	//načtení preferencí směn sestřičkou
    	for(int i=0;i<NUMBER_OF_SHIFTS-1;i++){
    		getline(input_file,s,',');
    		aa[i]=stoi(s);
    	}
    	getline(input_file,s);
		aa[NUMBER_OF_SHIFTS-1]=stoi(s);
    	
    	number_of_nurses++;
    	
    	//výpočet preferencí vzorů směn sestřičkou
    	int* pp=new int[NUMBER_OF_PATTERNS];
    	for(int i=0;i<NUMBER_OF_PATTERNS;i++){
    		int sum=0;
    		int number=0;
    		bitset<NUMBER_OF_SHIFTS> pattern=a[i];
    		for(int j=0;j<NUMBER_OF_SHIFTS;j++){
    			if(pattern[j]){
    				number++;
    				sum+=aa[j];
    			}
    		}
    		pp[i]=sum/number;
    	}
    	p.push_back(pp);
    }
    delete[] aa;
}

/*
	Uvolní data o sestřičkách.
*/
void free_nurses(){
	for(vector<int*>::iterator i=p.begin();i<p.end();i++){
		delete[] *i;
	}
}

/*
	Vytvoří prázdný rozvrh.
	return rozvrh
*/
int* get_empty_schedule(){
	int* r=new int[number_of_nurses];
	for(int i=0;i<number_of_nurses;i++){
		r[i]=-1;
	}
	return r;
}

/*
	Uvolní rozvrh.
*/
void free_schedule(int* s){
	delete[] s;
}

/*
	Inicializuje genom pro tvorbu počáteční populace na náhodnou permutaci
	čísel od 0 do number_of_nurses-1 včetně.
	g genom
*/
void init_genome(GAGenome& g){
	GARandomSeed();
	GA1DArrayGenome<int> &genome = (GA1DArrayGenome<int> &) g;
	std::vector<int> a(number_of_nurses);
	
	for(int i=0;i<number_of_nurses;i++){
		a[i]=i;
	}
  
	for(int i=0;i<number_of_nurses;i++){
		int c=GARandomInt(0, number_of_nurses-1-i);
		genome[i]=a[c];
		a.erase(a.begin()+c);
	}
}

/*
	Vytvoří rozvrh podle hloupého dekodéru.
	genome genom
	schedule rozvrh
*/
void dummy_decoder(GA1DArrayGenome<int>& genome, int* schedule){
	for(int i=0;i<number_of_nurses;i++){
		schedule[i]=dummy_pattern;
	}
}

/*
	Zjistí nejméně obsazené směny podle rozvrhu a stupně kvalifikace.
	Pokud je stupeň kvalifikace větší než 0 a všechny směny mají dostatek
	personálu vrátí se pole nul.
	1 ve výsledku znamená, že tato směna je nejméně obsazená.
	schedule rozvrh
	grade stupeň kvalifikace
	return nejméně obsazené směny
*/
bitset<NUMBER_OF_SHIFTS> least_covered_shifts(int* schedule, int grade){
	//minimální směny
	bitset<NUMBER_OF_SHIFTS> min_shifts=bitset<NUMBER_OF_SHIFTS>(0);
	//počet minimálních směn
	int min_cover=numeric_limits<int>::max();
	
	for(int i=0;i<NUMBER_OF_SHIFTS;i++){
		int cover=-R[i][grade];
		for(int j=0;j<number_of_nurses;j++){
			if(schedule[j]!=-1 && q[j][grade] && a[schedule[j]][i]){
				cover++;
			}
		}
		if(cover==min_cover){
			min_shifts[i]=1;
		}
		else if(cover<min_cover){
			min_cover=cover;
			min_shifts.reset();
			min_shifts[i]=1;
		}
	}
	
	if(grade>0 && min_cover>=0){
		return bitset<NUMBER_OF_SHIFTS>(0);
	}
	else{
		return min_shifts;
	}
}

/*
	Zjistí vzory směn, které pokrývají dané směny.
	shifts směny
	return vektor vzorů
*/
vector<int> patterns_covering_shifts(bitset<NUMBER_OF_SHIFTS> shifts){
	vector<int> best_patterns(0);
	int best_cover=numeric_limits<int>::min();
	for(int i=0;i<NUMBER_OF_PATTERNS;i++){
		int cover=(a[i]&shifts).count();
		if(cover>best_cover){
			best_cover=cover;
			best_patterns.clear();
			best_patterns.push_back(i);
		}
		else if(cover==best_cover){
			best_patterns.push_back(i);
		}
	}
	return best_patterns;
}

/*
	Vybere vzor ze seznamu vzorů, který je nejvhodnější pro práci sestřičky.
	nurse index sestričky
	patterns seznam vzorů
	return index vzoru
*/
int best_pattern(int nurse, vector<int> patterns){
	int best_pattern=-1;
	int best_pattern_score=numeric_limits<int>::max();
	
	for(unsigned int i=0;i<patterns.size();i++){
		int pattern=patterns[i];
		int pattern_score=p[nurse][pattern];
		if(pattern_score<best_pattern_score){
			best_pattern_score=pattern_score;
			best_pattern=pattern;
		}
	}
	
	return best_pattern;
}

/*
	Vytvoří rozvrh podle pokrývacího dekodéru.
	genome genom
	schedule rozvrh
	plus použít vylepšený výběr
*/
void cover_decoder(GA1DArrayGenome<int>& genome, int* schedule, bool plus){
	//pokrývání probíhá od nejvyšší kvalifikace
	for(int k=NUMBER_OF_GRADES-1;k>=0;k--){
		for(int i=0;i<number_of_nurses;i++){
			int nurse=genome[i];
			//pro každou sestřičku, která má odpovídající kvalifikaci a nemá
			//přiřazený vzor směn - přiřadím jí vzor směn
			if(q[nurse][k] && schedule[nurse]==-1){
				bitset<NUMBER_OF_SHIFTS> shifts=least_covered_shifts(schedule, k);
				//pokud jsou všechny směny již obsazené začnou se obsazovat
				//místa s nižším požadovaným stupněm kvalifikace
				if(shifts.count()==0){
					break;
				}
				else{
					int pattern=0;
					if(plus){
						pattern=best_pattern(nurse, patterns_covering_shifts(shifts));
					}
					else{
						pattern=patterns_covering_shifts(shifts)[0];
					}
					schedule[nurse]=pattern;
				}
			}
		}
	}
}

/*
	Zjistí počet neobsazených míst v jednotlivých směnách.
	Pokud je binary false: Do r[i][j] se uloží počet neobsazených míst ve
	směně i podle stupně kvalifikace j, nebo 0 pokud jsou všecna místa obsazená.
	Pokud je binary true použije se binární reprezentace: Do r[i][j] se uloží 1
	pokud jsou ve směně i podle stupně kvalifikace j neobsazená místa, jinak 0.
	schedule rozvrh
	r výsledek
	binary má se použít binární reprezentace
*/
void noncovered_shifts(int* schedule, int** r, bool binary){
	//inicializace r
	for(int i=0;i<NUMBER_OF_SHIFTS;i++){
		for(int j=0;j<NUMBER_OF_GRADES;j++){
			r[i][j]=R[i][j];
		}
	}
	
	//výpočet r
	for(int i=0;i<number_of_nurses;i++){
		if(schedule[i]!=-1){
			bitset<NUMBER_OF_SHIFTS> pattern=a[schedule[i]];
			for(int j=0;j<NUMBER_OF_SHIFTS;j++){
				if(pattern[j]){
					for(int k=0;k<NUMBER_OF_GRADES;k++){
						if(q[i][k]){
							r[j][k]--;
						}
					}
				}
			}
		}
	}
	
	//úprava r - negativní hodnoty na 0
	for(int i=0;i<NUMBER_OF_SHIFTS;i++){
		for(int j=0;j<NUMBER_OF_GRADES;j++){
			if(binary){
				r[i][j]=r[i][j]<=0?0:1;
			}
			else{
				r[i][j]=r[i][j]<=0?0:r[i][j];
			}
		}
	}
}

/*
	Vytvoří rozvrh podle přízpěvkového dekodéru.
	genome genom
	schedule rozvrh
	binary použít binární reprezentaci
*/
void contribution_decoder(GA1DArrayGenome<int>& genome, int* schedule, bool binary){
	//skóre jednotlivých vzorů
	int* scores=new int[NUMBER_OF_PATTERNS];
	//počet neobsazených míst v jednotlivých směnách
	int** d=new int*[NUMBER_OF_SHIFTS];
	
	//alokace d
	for(int i=0;i<NUMBER_OF_SHIFTS;i++){
		d[i]=new int[NUMBER_OF_GRADES];
	}
	
	//každé sestřičce je přidělen vzor s nejlepším skóre
	for(int i=0;i<number_of_nurses;i++){
		int nurse=genome[i];
		//výpočet skóre vzorů
		noncovered_shifts(schedule,d,binary);
		for(int j=0;j<NUMBER_OF_PATTERNS;j++){
			int score=PREFERENCE_WEIGHT*(100-p[nurse][j]);
			for(int s=0;s<NUMBER_OF_GRADES;s++){
				int subscore=0;
				for(int k=0;k<NUMBER_OF_SHIFTS;k++){
					subscore+=a[j][k]*d[k][s];
				}
				score+=UNCOVERED_WEIGHT[s]*q[nurse][s]*subscore;
			}
			scores[j]=score;
		}
		
		//výběr vzoru s největším skóre
		int best_score=numeric_limits<int>::min();
		for(int j=0;j<NUMBER_OF_PATTERNS;j++){
			if(scores[j]>best_score){
				best_score=scores[j];
				schedule[nurse]=j;
			}
		}
	}
	
	delete[] scores;
	
	for(int i=0;i<NUMBER_OF_SHIFTS;i++){
		delete[] d[i];
	}
	delete[] d;
}

/*
	Vytvoří rozvrh podle vybraného dekodéru.
	genome genom
	schedule rozvrh
*/
void decode(GA1DArrayGenome<int>& genome, int* schedule){
	switch(decoder){
		case DUMMY: dummy_decoder(genome,schedule);break;
		case COVER: cover_decoder(genome,schedule, false);break;
		case COVER_PLUS: cover_decoder(genome,schedule,true);break;
		case CONTRIBUTION: contribution_decoder(genome,schedule,true);break;
		case COMBINED: contribution_decoder(genome,schedule,false);break;
	}
}

/*
	Vypočítá skóre rozvrhu.
	x rozvrh
	return skóre rozvrhu
*/
float score_schedule(int* x){
	int e=0;
	int f=0;
	int g=0;
	
	for(int i=0;i<number_of_nurses;i++){
		e+=p[i][x[i]];
	}
	
	for(int k=0;k<NUMBER_OF_SHIFTS;k++){
		for(int s=0;s<NUMBER_OF_GRADES;s++){
			f=R[k][s];
			
			for(int i=0;i<number_of_nurses;i++){
				f-=q[i][s]*a[x[i]][k];
			}
			
			f=f<0?0:f;
			g+=f;
		}
	}
	
	return e+WEIGHT*g;
}

/*
	Vytiskne rozvrh.
	x rozvrh
*/
void print_schedule(int* x){
	//seznam seznamů jmen sestřiček pracující ve směne
	vector<string>* v=new vector<string>[NUMBER_OF_SHIFTS];
	for(int i=0;i<NUMBER_OF_SHIFTS;i++){
		for(int j=0;j<number_of_nurses;j++){
			if(a[x[j]][i]){
				v[i].push_back(names[j]);
			}
		}
	}
	
	//délka nejdelšího jména sestřičky nebo 5, pokud je 5 větší
	int longest=5;
	for(int i=0;i<number_of_nurses;i++){
		if(((int)names[i].size())>longest){
			longest=names[i].size();
		}
	}
	
	longest++;
	//šířka tabulky
	int width=2*longest+3;
	
	//tisk horizontální čáry
	for(int k=0;k<width;k++){
		cout<<"-";
	}
	cout<<endl;
	
	for(int i=0;i<DAYS_IN_WEEK;i++){
		//počet potřebných řádků pro tisk rozvrhu na itý den
		int size=v[i].size()>v[i+7].size()?v[i].size():v[i+7].size();
		//tisk jmen
		for(int j=0;j<size;j++){
			cout<<"|";
			//tisk jména doplněného mezerami, pokud se nepovede tisk mezer
			try{
				cout<<v[i].at(j);
				for(int k=0;k<longest-((int)v[i].at(j).size());k++){
					cout<<" ";
				}
			}
			catch(out_of_range e){
				for(int k=0;k<longest;k++){
					cout<<" ";
				}
			}
			
			cout<<"|";
			
			//tisk jména doplněného mezerami, pokud se nepovede tisk mezer
			try{
				cout<<v[i+7].at(j);
				for(int k=0;k<longest-((int)v[i+7].at(j).size());k++){
					cout<<" ";
				}
			}
			catch(out_of_range e){
				for(int k=0;k<longest;k++){
					cout<<" ";
				}
			}
			
			cout<<"|";
			cout<<endl;
		}
		
		//tisk horizontální čáry
		for(int k=0;k<width;k++){
			cout<<"-";
		}
		cout<<endl;
	}
	
	delete[] v;
}

/*
	Tisk genomu.
	g genom
*/
void print_genome(GA1DArrayGenome<int> &g){
	for(int i=0;i<number_of_nurses;i++){
		cout<<names[g[i]]<<" ";
	}
	cout<<endl;
}

/*
	Fitness funkce.
	g genom
	return fitness genomu
*/
float fitness(GAGenome& g){
	float score=0.0;
	GA1DArrayGenome<int> &genome=(GA1DArrayGenome<int>&)g;
	int* schedule=get_empty_schedule();
	
	decode(genome,schedule);
	
	score=score_schedule(schedule);
	free_schedule(schedule);
	return score;
}

/*
	Zjistí jestli je rozvrh vhodný.
	schedule rozvrh
	return možnost
*/
bool is_feasible(int* schedule){
	//počet neobsazených míst v jednotlivých směnách
	int** d=new int*[NUMBER_OF_SHIFTS];
	for(int i=0;i<NUMBER_OF_SHIFTS;i++){
		d[i]=new int[NUMBER_OF_GRADES];
	}
	
	noncovered_shifts(schedule,d,true);
	
	//pokud existuje neobsazená směna vrátí false
	for(int i=0;i<NUMBER_OF_SHIFTS;i++){
		for(int j=0;j<NUMBER_OF_GRADES;j++){
			if(d[i][j]){
				for(int i=0;i<NUMBER_OF_SHIFTS;i++){
					delete[] d[i];
				}
				delete[] d;
				return false;
			}
		}
	}
	
	//úklid
	for(int i=0;i<NUMBER_OF_SHIFTS;i++){
		delete[] d[i];
	}
	delete[] d;
	
	return true;
}

/*
	Tisk nápovědy.
*/
void print_help(){
	cout
	<<"Nurse scheduling problem solvong using genetic al"<<endl
	<<"Parameters:"<<endl
	<<"    -h        Show this message."<<endl
	<<"    -t        Print shortened testing output."<<endl
	<<"    -n FILE   Nurses data file."<<endl
	<<"    -r FILE   Requirements data file."<<endl
	<<"    -d DEC    Use decoder DEC."<<endl
	<<"    -p PN     Dummy decoder should use pattern PN."<<endl
	<<"Decoders:"<<endl
	<<"    dummy, cover, cover+, contribution, combined"<<endl;
}

/*
	Provede naparsování argumentů.
	argc počet argumentů příkazové řádky
	argv argumenty příkazové řádky
*/
Args parse_arguments(int argc, char** argv){
	Args r;
	r.error=false;
	r.nurses_file=DEFAULT_NURSES_FILE;
	r.requirements_file=DEFAULT_REQUIREMENTS_FILE;
	r.testing=false;
	r.help=false;
	r.crossover=DEFAULT_CROSSOVER;
	int c=-1;
	while((c=getopt(argc,argv,"thn:r:d:p:x:"))!=-1){
		switch(c){
			//výpis pro testování
			case 't':{
				r.testing=true;
			}
			break;
			//výpis nápovědy
			case 'h':{
				r.help=true;
			}
			break;
			//soubor s daty o sesřičkách
			case 'n':{
				r.nurses_file=string(optarg);
				ifstream f(r.nurses_file);
				if(!f){
					r.error=true;
					cerr<<"Nurses file does not exists."<<endl;
				}
			}
			break;
			//soubor s požadavky
			case 'r':{
				r.requirements_file=string(optarg);
				ifstream f(r.requirements_file);
				if(!f){
					r.error=true;
					cerr<<"Requirements file does not exists."<<endl;
				}
			}
			break;
			//vzor pro hloupý dekodér
			case 'p':{
				int number=stoi(string(optarg));
				if(number<0 || number>=NUMBER_OF_PATTERNS){
					r.error=true;
					cerr<<"Illegal pattern number."<<endl;
				}
				else{
					dummy_pattern=number;
				}
			}
			break;
			//výběr dekodéru
			case 'd':{
				if(strcmp(optarg,"dummy")==0){
					decoder=DUMMY;
				}
				else if(strcmp(optarg,"cover")==0){
					decoder=COVER;
				}
				else if(strcmp(optarg,"cover+")==0){
					decoder=COVER_PLUS;
				}
				else if(strcmp(optarg,"contribution")==0){
					decoder=CONTRIBUTION;
				}
				else if(strcmp(optarg,"combined")==0){
					decoder=COMBINED;
				}
				else{
					cerr<<"Uknown decoder."<<endl;
					r.error=true;
				}
			}
			break;
			//výběr křížení
			case 'x':{
				if(strcmp(optarg,"pmx")==0){
					r.crossover=PMX;
				}
				else if(strcmp(optarg,"order")==0){
					r.crossover=ORDER;
				}
				else{
					cerr<<"Uknown crossover."<<endl;
					r.error=true;
				}
			}
			break;
		}
	}
	return r;
}

/*
	Hlavní funkce.
	argc počet argumentů příkazové řádky
	argv argumenty příkazové řádky
*/
int main(int argc, char** argv){
	//parsování argumentů
	Args args=parse_arguments(argc, argv);
	
	//vypsání nápovědy
	if(args.help){
		print_help();
		return EXIT_SUCCESS;
	}
	
	//pokud nastala chyba při parsování nápovědy
	if(args.error){
		return EXIT_FAILURE;
	}
	
	//vytvoření potřebných struktur
	create_patterns();
	load_nurses(args.nurses_file);
	load_requirements(args.requirements_file);
	
	//nastavení GA
	GA1DArrayGenome<int> genome(number_of_nurses,fitness);
	genome.initializer(init_genome);
   	genome.mutator(GA1DArrayGenome<int>::SwapMutator);
	switch(args.crossover){
		case PMX:genome.crossover(GA1DArrayGenome<int>::PartialMatchCrossover);break;
		case ORDER:genome.crossover(GA1DArrayGenome<int>::OrderCrossover);break;
	}
	GASimpleGA ga(genome);
	ga.minimize();
	ga.parameters(argc,argv);
	ga.initialize();
	
	//běh GA
	while(!ga.done()){
		cout.flush();
		++ga;
		if(ga.population().min()==0.0){
			break;
		}
	}
	ga.flushScores();
	
	//vybrání/tvorba nejlepšího řešení
	GA1DArrayGenome<int> &g=(GA1DArrayGenome<int>&)ga.statistics().bestIndividual();
	int* schedule=get_empty_schedule();
	decode(g,schedule);
	
	//výpis výsledků pro testování
	if(args.testing){
		cout<<ga.population().min()<<" "<<is_feasible(schedule)<<endl;
	}
	//běžný výpis výsledků
	else{
		cout<<endl<<"Final statistics:"<<endl;
		cout<<ga.statistics()<<endl;
		cout<<"The best solution: "<<ga.statistics().bestIndividual()<<" , Fitness = "<<ga.population().min()<<" , Feasible = "<<is_feasible(schedule)<<endl;
		
		print_genome(g);
		print_schedule(schedule);
	}
	
	//úklid
	free_schedule(schedule);
	free_patterns();
	free_nurses();
	free_requirements();
	
	return EXIT_SUCCESS;
}
