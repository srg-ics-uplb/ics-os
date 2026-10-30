#!/bin/bash

#for markdown with special characters
#pandoc ${1}.md -o ${1}.pdf --pdf-engine=xelatex --template "./eisvogel.tex" -V mainfont="Liberation Serif" -V monofont="Liberation Mono" --highlight-style zenburn && evince ${1}.pdf &


#for markdown with codes
#pandoc ${1}.md -o ${1}.pdf --pdf-engine=xelatex --template "./eisvogel.tex" -V mainfont="Liberation Serif" -V monofont="IBM Plex Sans Condensed" --highlight-style zenburn && evince ${1}.pdf &
pandoc ${1}.md -o ${1}.pdf --pdf-engine=xelatex --template "./eisvogel.tex" -V mainfont="Liberation Serif" -V monofont="IBM Plex Sans Condensed" --highlight-style zenburn

