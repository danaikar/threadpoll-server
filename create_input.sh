#!/bin/bash
 
# check number of arguments 
if [ $# -ne 2 ]; then
    echo "Give the correct number of arguments."
    exit 1
fi

# check for the txt file
if [ ! -f "$1" ]; then
    echo "File $1 doesn't exists."
    exit 1
fi

# check if numLines is valid
re='^[0-9]+$'
if ! [[ $2 =~ $re ]]; then
    echo "Give a valid number of lines."
    exit 1
fi


# store the content of politicalParties.txt in an array called 'parties'
mapfile -t parties < "$1"

# create the inputFile
inputFile="inputFile.txt"
> "$inputFile"


# loop numLines to create the inputFile
for (( i=1; i<=$2; i++ )); do
    # choose a random name and surname (first letter capitalized and the rest of the letters in lowercase)
    randomName=$(cat /dev/urandom | tr -dc 'A-Z' | fold -w "$(shuf -i 3-12 -n 1)" | head -n 1)
    randomName=${randomName:0:1}$(echo "${randomName:1}" | tr 'A-Z' 'a-z')

    randomSurname=$(cat /dev/urandom | tr -dc 'A-Z' | fold -w "$(shuf -i 3-12 -n 1)" | head -n 1)
    randomSurname=${randomSurname:0:1}$(echo "${randomSurname:1}" | tr 'A-Z' 'a-z')
    
    # choose a random party
    randomParty=${parties[$(shuf -i 0-$(( ${#parties[@]} - 1 )) -n 1)]}
    
    # write the random result in the inputFile
    echo "$randomName $randomSurname, $randomParty" >> "$inputFile"
done

echo "The $inputFile has been created."