/*
 * (C) Copyright 2001 Diomidis Spinellis.
 *
 * Portable graph display abstraction
 *
 * $Id: gdisplay.h,v 1.4 2006/06/13 21:43:33 dds Exp $
 */


// Abstract base class, used for drawing
class GraphDisplay {
protected:
	FILE *fo;
public:
	GraphDisplay(FILE *f) : fo(f) {}
	virtual void head(const char *fname, const char *title) {};
	virtual void subhead(const string &text) {};
	virtual void node(Call *p) {};
	virtual void edge(Call *a, Call *b) = 0;
	virtual void tail() {};
	virtual ~GraphDisplay() {}
};

// HTML output
class GDHtml: public GraphDisplay {
public:
	GDHtml(FILE *f) : GraphDisplay(f) {}
	virtual void head(const char *fname, const char *title) {
		html_head(fo, fname, title);
		fprintf(fo, "<table border=\"0\">\n");
	}

	virtual void subhead(const string &text) {
		fprintf(fo, "<h2>%s</h2>\n", text.c_str());
	}

	virtual void edge(Call *a, Call *b) {
		fprintf(fo,
		    "<tr><td align=\"right\">%s</td><td>&rarr;</td><td>%s</td></tr>\n",
		    function_label(a, true).c_str(),
		    function_label(b, true).c_str());
	}

	virtual void tail() {
		fprintf(fo, "</table>\n");
		html_tail(fo);
	}
	virtual ~GDHtml() {}
};

// Raw text output
class GDTxt: public GraphDisplay {
public:
	GDTxt(FILE *f) : GraphDisplay(f) {}
	virtual void edge(Call *a, Call *b) {
		fprintf(fo, "%s %s\n",
		    function_label(a, false).c_str(),
		    function_label(b, false).c_str());
	}
	virtual ~GDTxt() {}
};

// AT&T GraphViz Dot output
class GDDot: public GraphDisplay {
public:
	GDDot(FILE *f) : GraphDisplay(f) {}
	virtual void head(const char *fname, const char *title) {
		fprintf(fo, "#!/usr/local/bin/dot\n"
			"#\n# Generated by CScout %s - %s\n#\n\n"
			"digraph G {\n",
			Version::get_revision().c_str(),
			Version::get_date().c_str());
		if (cgraph_show == 'e')			// Empty nodes
			fprintf(fo, "\tnode [height=.001,width=0.000001,shape=box,label=\"\",fontsize=8];\n");
	}

	virtual void node(Call *p) {
		fprintf(fo, "\t_%p [label=\"%s\"", p, function_label(p, false).c_str());
		if (isHyperlinked())
			fprintf(fo, ", URL=\"http://localhost:%d/fun.html?f=%p\"", portno, p);
		fprintf(fo, "];\n");
	}

	virtual void edge(Call *a, Call *b) {
		fprintf(fo, "\t_%p -> _%p;\n", a, b);
	}

	virtual void tail() {
		fprintf(fo, "}\n");
	}
	virtual bool isHyperlinked() { return (false); }
	virtual ~GDDot() {}
};

// Generate a graph of the specified format by calling dot
class GDDotImage: public GDDot {
private:
	char img[256];		// Image file name
	char dot[256];		// dot file name
	char cmd[1024];		// dot command
	char *format;		// Output format
	FILE *result;		// Resulting image
public:
	GDDotImage(FILE *f, char *fmt) : GDDot(NULL), format(fmt), result(f) {}

	void head(const char *fname, const char *title) {
		#if defined(unix) || defined(__MACH__)
		strcpy(img, "/tmp");
		#elif defined(WIN32)
		char *tmp = getenv("TEMP");
		strcpy(img, tmp ? tmp : ".");
		#else
		#error "Don't know how to obtain temporary directory"
		#endif
		strcpy(dot, img);
		strcat(dot, "/CSdot-XXXXXX");
		strcat(img, "/CSimg-XXXXXX");
		mktemp(dot);
		mktemp(img);
		fo = fopen(dot, "w");
		if (fo == NULL) {
			html_perror(fo, "Unable to open " + string(dot) + " for writing", true);
			return;
		}
		GDDot::head(fname, title);
	}

	virtual void tail() {
		GDDot::tail();
		fclose(fo);
		snprintf(cmd, sizeof(cmd), "dot -T%s \"%s\" \"-o%s\"", format, dot, img);
		if (DP())
			cout << cmd << '\n';
		if (system(cmd) != 0) {
			html_perror(fo, "Unable to execute " + string(cmd) + ". Shell execution", true);
			return;
		}
		FILE *fimg = fopen(img, "rb");
		if (fimg == NULL) {
			html_perror(fo, "Unable to open " + string(img) + " for reading", true);
			return;
		}
		int c;
		#ifdef WIN32
		setmode(fileno(result), O_BINARY);
		#endif
		while ((c = getc(fimg)) != EOF)
			putc(c, result);
		fclose(fimg);
		//(void)unlink(dot);
		//(void)unlink(img);
	}
	virtual ~GDDotImage() {}
};

// SVG via dot
class GDSvg: public GDDotImage {
public:
	GDSvg(FILE *f) : GDDotImage(f, "svg") {}
	virtual bool isHyperlinked() { return (true); }
	virtual ~GDSvg() {}
};

// GIF via dot
class GDGif: public GDDotImage {
public:
	GDGif(FILE *f) : GDDotImage(f, "gif") {}
	virtual ~GDGif() {}
};
