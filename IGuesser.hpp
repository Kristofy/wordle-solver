#pragma once
#include <vector>
#include <string>

class IGuesser
{
protected:
    std::vector <std::string> fiveLetterWords;
public:
    enum LetterResult {
        Green,
        Yellow,
        Grey
    };
    
    IGuesser (const std::vector <std::string>& fiveLetterWords) :fiveLetterWords(fiveLetterWords.begin(), fiveLetterWords.end()) {}
    virtual void Reset () {}
    virtual std::string MakeAGuess (std::vector <std::vector <LetterResult>> resultsOfFormerGuesses) = 0;
    virtual ~IGuesser () {}
};