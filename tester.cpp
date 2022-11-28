#include "Guesser.hpp"
#include "IGuesser.hpp"
#include <iostream>
#include <fstream>
#include <random>
#include "timer.hpp"


using namespace std;



std::ostream& operator<<(std::ostream& os, const IGuesser::LetterResult& l){
    static const std::string greenfg = "\033[32m";
    static const std::string yellowfg = "\033[33m";
    static const std::string blackfg = "\033[30m";
    static const std::string colors[3] = {greenfg, yellowfg, blackfg};
    static const std::string marks[3] = {"▮","▮","▮"};

    os << colors[l] << marks[l];
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<IGuesser::LetterResult>& v){
    static const std::string whitebg = "\033[47m";
    static const std::string reset = "\033[0m";

    os << whitebg;
    for(const auto& x : v) os << x;

    os << reset;
    return os;
}


vector<string> readFile(const string& filename){
    vector<string> words;
    ifstream fin(filename);
    string word;
    while(fin >> word){
        if(word.length() != 5) throw "Read a non 5 letter word";
        words.push_back(word);
    }
    fin.close();
    sort(words.begin(), words.end());
    return words;
}

string pickRandom(const vector<string>& words){
    static auto seed = 0;//time(0);
    static mt19937 rng(seed);

    return words[rng() % words.size()];
}

vector<IGuesser::LetterResult> evaulateGuess(const string& guess, const string& target){
    if(guess.length() !=  5 || target.length() != 5) throw "something went wrong";
    vector<IGuesser::LetterResult> res(5,IGuesser::LetterResult::Grey);
    vector<int> w(26, 0);
    for(int i = 0; i < 5; i++){
        if (guess[i] == target[i]) {
            w[guess[i]-'a']++;
            res[i] = IGuesser::LetterResult::Green; 
        }
    }

    for(int i = 0; i < 5; i++){
        if(res[i] == IGuesser::LetterResult::Green) continue;
        int k = w[guess[i]-'a']++; // eddig volt k darab ilyen betű
        int in = 0;
        for(int j = 0; j < 5; j++){
            if(guess[i] == target[j]) { in++; }
        }
        // ha többször van benne mint ahány ilyen betűt találtunk eddig
        if(k < in){ res[i] = IGuesser::LetterResult::Yellow; }
        else { res[i] = IGuesser::LetterResult::Grey; }
    }

    return res;
}

int main() {

    vector<string> words = readFile("test_words.txt");
    std::random_device rd;
    std::mt19937 g(rd());
    shuffle(words.begin(), words.end(), g);

    const vector<string> word_list(words.begin(), words.begin()+10000); 
    {
        // TimeScope("Guesser");

        TimeScope("total");
        Guesser g(word_list);
        int sum = 0;
        for(int i = 0; i < 10000; i++){
            // TimeScope("Guess")
            g.Reset();
            const string target = pickRandom(word_list);
            std::cout << "########### NEW GAME ###########\n";
            std::cout << "target: " << target << endl;
            vector<vector<IGuesser::LetterResult>> guessResults;
            string guess = "";
            int cnt = 0;
            while(guess != target){
                cnt++;
                guess = g.MakeAGuess(guessResults);
                guessResults.push_back(evaulateGuess(guess, target));
                std::cout << guessResults.back() << ": " << guess << std::endl;
            }
            sum += cnt;

             cout << "Guessed in: " << cnt << endl;


        }
            cout << "AVG: " << (double)sum/(10) << endl;


    }

    return 0;

}