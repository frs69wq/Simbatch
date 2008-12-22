SOURCE		:= simbatch.tex
FIGFILES	:= $(wildcard figures/*.fig)
PDFFIGFILES	:= $(FIGFILES:%.fig=%.pdf)

all: pdf

pdf: $(SOURCE) $(PDFFIGFILES)
	pdflatex $(SOURCE)
	bibtex simbatch.aux
	pdflatex $(SOURCE)
	pdflatex $(SOURCE)

$(PDFFIGFILES): %.pdf: %.fig
	fig2dev -L pdf $*.fig > $*.pdf	
	
clean:
	rm -rf *.aux *.log auto *~ $(SOURCE:%.tex=%.pdf) $(PDFFIGFILES) *.out \
	*.toc *.lof *.lot *.blg *.bbl
