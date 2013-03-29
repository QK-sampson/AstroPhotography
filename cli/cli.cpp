/*
 * cli.cpp -- embeddable command line interpreter
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <cli.h>
#include <FlexLexer.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <variables.h>

int	yyFlexLexer::yywrap() {
	return 1;
} 

// the scanner must be instantiated in some other file
FlexLexer	*astro_scanner = NULL;

/**
 * \brief Call the lexer
 *
 * The parser wants to call the lexer by simply invoking the yylex() function.
 * However, the lexer is really an instance of yyFlexLexer, so we have to
 * provide a wrapper function that calls the yylex method of the scanner class.
 * to find the currently active scanner we have to use the global variable
 * astro_scanner.
 */
int	yylex() {
	if (NULL == astro_scanner) {
		return EOF;
	}
	return astro_scanner->yylex();
}

/**
 * \brief Error message function
 *
 * Again
 */
int yyerror(char *s) {
	fprintf(stderr, "error on line %d: %s\n", astro_scanner->lineno(), s);
}

/* prototype for the yyparse function */
extern int yyparse();

// everything else is inside the astro::cli namespace
namespace astro {
namespace cli {

/**
 * \brief Parse a script
 *
 * This method accepts a file name (the standard input is used if the file 
 * name is NULL) and parses the contents of the file as a command line script.
 */
int	cli::parse(const char *filename) {
	// construct istream depending on filename: if filen == NULL, take
	// std::cin, otherwise open the file as a std::ifstream
	std::istream	*input = NULL;
	if (NULL != filename) {
		input = new std::ifstream(filename);
		if (NULL == input) {
			std::cerr << "cannot open " << filename << std::endl;
			throw std::invalid_argument("cannot open input file");
		}
	} else {
		input = &std::cin;
	}

	// create a scanner with this input as the input channel
	astro_scanner = new yyFlexLexer(input);
	int	result = yyparse();

	// scanner cleanup 
	delete astro_scanner;
	astro_scanner = NULL;

	// return the result of the parse operation
	return result;
}

std::ostream&   operator<<(std::ostream& out, const cli& c) {
	out << "cli has " << c.vars.size() << " variables" << std::endl;
	return out << c.vars;
}

cli	*sharedcli::c = NULL;

sharedcli::sharedcli() {
	if (NULL == c) {
		c = new cli();
	}
}

sharedcli::sharedcli(cli *_c) { c = _c; }

int	sharedcli::parse(const char *filename) {
	c->parse(filename);
}

std::ostream&   operator<<(std::ostream& out, const sharedcli& c) {
	return out << *c.c;
}

} // namespace cli
} // namespace astro
