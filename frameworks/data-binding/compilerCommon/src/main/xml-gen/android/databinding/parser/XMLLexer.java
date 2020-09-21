// Generated from XMLLexer.g4 by ANTLR 4.5.3
package android.databinding.parser;
import org.antlr.v4.runtime.Lexer;
import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.Token;
import org.antlr.v4.runtime.TokenStream;
import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.atn.*;
import org.antlr.v4.runtime.dfa.DFA;
import org.antlr.v4.runtime.misc.*;

@SuppressWarnings({"all", "warnings", "unchecked", "unused", "cast"})
public class XMLLexer extends Lexer {
	static { RuntimeMetaData.checkVersion("4.5.3", RuntimeMetaData.VERSION); }

	protected static final DFA[] _decisionToDFA;
	protected static final PredictionContextCache _sharedContextCache =
		new PredictionContextCache();
	public static final int
		COMMENT=1, CDATA=2, DTD=3, EntityRef=4, CharRef=5, SEA_WS=6, OPEN=7, XMLDeclOpen=8, 
		TEXT=9, CLOSE=10, SPECIAL_CLOSE=11, SLASH_CLOSE=12, SLASH=13, EQUALS=14, 
		STRING=15, Name=16, S=17, PI=18;
	public static final int INSIDE = 1;
	public static final int PROC_INSTR = 2;
	public static String[] modeNames = {
		"DEFAULT_MODE", "INSIDE", "PROC_INSTR"
	};

	public static final String[] ruleNames = {
		"COMMENT", "CDATA", "DTD", "EntityRef", "CharRef", "SEA_WS", "OPEN", "XMLDeclOpen", 
		"SPECIAL_OPEN", "TEXT", "CLOSE", "SPECIAL_CLOSE", "SLASH_CLOSE", "SLASH", 
		"EQUALS", "STRING", "Name", "S", "HEXDIGIT", "DIGIT", "NameChar", "NameStartChar", 
		"PI", "IGNORE"
	};

	private static final String[] _LITERAL_NAMES = {
		null, null, null, null, null, null, null, "'<'", null, null, "'>'", null, 
		"'/>'", "'/'", "'='"
	};
	private static final String[] _SYMBOLIC_NAMES = {
		null, "COMMENT", "CDATA", "DTD", "EntityRef", "CharRef", "SEA_WS", "OPEN", 
		"XMLDeclOpen", "TEXT", "CLOSE", "SPECIAL_CLOSE", "SLASH_CLOSE", "SLASH", 
		"EQUALS", "STRING", "Name", "S", "PI"
	};
	public static final Vocabulary VOCABULARY = new VocabularyImpl(_LITERAL_NAMES, _SYMBOLIC_NAMES);

	/**
	 * @deprecated Use {@link #VOCABULARY} instead.
	 */
	@Deprecated
	public static final String[] tokenNames;
	static {
		tokenNames = new String[_SYMBOLIC_NAMES.length];
		for (int i = 0; i < tokenNames.length; i++) {
			tokenNames[i] = VOCABULARY.getLiteralName(i);
			if (tokenNames[i] == null) {
				tokenNames[i] = VOCABULARY.getSymbolicName(i);
			}

			if (tokenNames[i] == null) {
				tokenNames[i] = "<INVALID>";
			}
		}
	}

	@Override
	@Deprecated
	public String[] getTokenNames() {
		return tokenNames;
	}

	@Override

	public Vocabulary getVocabulary() {
		return VOCABULARY;
	}


	public XMLLexer(CharStream input) {
		super(input);
		_interp = new LexerATNSimulator(this,_ATN,_decisionToDFA,_sharedContextCache);
	}

	@Override
	public String getGrammarFileName() { return "XMLLexer.g4"; }

	@Override
	public String[] getRuleNames() { return ruleNames; }

	@Override
	public String getSerializedATN() { return _serializedATN; }

	@Override
	public String[] getModeNames() { return modeNames; }

	@Override
	public ATN getATN() { return _ATN; }

	public static final String _serializedATN =
		"\3\u0430\ud6d1\u8206\uad2d\u4417\uaef1\u8d80\uaadd\2\24\u00e9\b\1\b\1"+
		"\b\1\4\2\t\2\4\3\t\3\4\4\t\4\4\5\t\5\4\6\t\6\4\7\t\7\4\b\t\b\4\t\t\t\4"+
		"\n\t\n\4\13\t\13\4\f\t\f\4\r\t\r\4\16\t\16\4\17\t\17\4\20\t\20\4\21\t"+
		"\21\4\22\t\22\4\23\t\23\4\24\t\24\4\25\t\25\4\26\t\26\4\27\t\27\4\30\t"+
		"\30\4\31\t\31\3\2\3\2\3\2\3\2\3\2\3\2\7\2<\n\2\f\2\16\2?\13\2\3\2\3\2"+
		"\3\2\3\2\3\3\3\3\3\3\3\3\3\3\3\3\3\3\3\3\3\3\3\3\3\3\7\3P\n\3\f\3\16\3"+
		"S\13\3\3\3\3\3\3\3\3\3\3\4\3\4\3\4\3\4\7\4]\n\4\f\4\16\4`\13\4\3\4\3\4"+
		"\3\4\3\4\3\5\3\5\3\5\3\5\3\6\3\6\3\6\3\6\6\6n\n\6\r\6\16\6o\3\6\3\6\3"+
		"\6\3\6\3\6\3\6\3\6\6\6y\n\6\r\6\16\6z\3\6\3\6\5\6\177\n\6\3\7\3\7\5\7"+
		"\u0083\n\7\3\7\6\7\u0086\n\7\r\7\16\7\u0087\3\b\3\b\3\b\3\b\3\t\3\t\3"+
		"\t\3\t\3\t\3\t\3\t\3\t\3\t\3\t\3\n\3\n\3\n\3\n\3\n\3\n\3\n\3\n\3\13\6"+
		"\13\u00a1\n\13\r\13\16\13\u00a2\3\f\3\f\3\f\3\f\3\r\3\r\3\r\3\r\3\r\3"+
		"\16\3\16\3\16\3\16\3\16\3\17\3\17\3\20\3\20\3\21\3\21\7\21\u00b9\n\21"+
		"\f\21\16\21\u00bc\13\21\3\21\3\21\3\21\7\21\u00c1\n\21\f\21\16\21\u00c4"+
		"\13\21\3\21\5\21\u00c7\n\21\3\22\3\22\7\22\u00cb\n\22\f\22\16\22\u00ce"+
		"\13\22\3\23\3\23\3\23\3\23\3\24\3\24\3\25\3\25\3\26\3\26\3\26\3\26\5\26"+
		"\u00dc\n\26\3\27\5\27\u00df\n\27\3\30\3\30\3\30\3\30\3\30\3\31\3\31\3"+
		"\31\3\31\5=Q^\2\32\5\3\7\4\t\5\13\6\r\7\17\b\21\t\23\n\25\2\27\13\31\f"+
		"\33\r\35\16\37\17!\20#\21%\22\'\23)\2+\2-\2/\2\61\24\63\2\5\2\3\4\f\4"+
		"\2\13\13\"\"\4\2((>>\4\2$$>>\4\2))>>\5\2\13\f\17\17\"\"\5\2\62;CHch\3"+
		"\2\62;\4\2/\60aa\5\2\u00b9\u00b9\u0302\u0371\u2041\u2042\n\2<<C\\c|\u2072"+
		"\u2191\u2c02\u2ff1\u3003\ud801\uf902\ufdd1\ufdf2\uffff\u00f3\2\5\3\2\2"+
		"\2\2\7\3\2\2\2\2\t\3\2\2\2\2\13\3\2\2\2\2\r\3\2\2\2\2\17\3\2\2\2\2\21"+
		"\3\2\2\2\2\23\3\2\2\2\2\25\3\2\2\2\2\27\3\2\2\2\3\31\3\2\2\2\3\33\3\2"+
		"\2\2\3\35\3\2\2\2\3\37\3\2\2\2\3!\3\2\2\2\3#\3\2\2\2\3%\3\2\2\2\3\'\3"+
		"\2\2\2\4\61\3\2\2\2\4\63\3\2\2\2\5\65\3\2\2\2\7D\3\2\2\2\tX\3\2\2\2\13"+
		"e\3\2\2\2\r~\3\2\2\2\17\u0085\3\2\2\2\21\u0089\3\2\2\2\23\u008d\3\2\2"+
		"\2\25\u0097\3\2\2\2\27\u00a0\3\2\2\2\31\u00a4\3\2\2\2\33\u00a8\3\2\2\2"+
		"\35\u00ad\3\2\2\2\37\u00b2\3\2\2\2!\u00b4\3\2\2\2#\u00c6\3\2\2\2%\u00c8"+
		"\3\2\2\2\'\u00cf\3\2\2\2)\u00d3\3\2\2\2+\u00d5\3\2\2\2-\u00db\3\2\2\2"+
		"/\u00de\3\2\2\2\61\u00e0\3\2\2\2\63\u00e5\3\2\2\2\65\66\7>\2\2\66\67\7"+
		"#\2\2\678\7/\2\289\7/\2\29=\3\2\2\2:<\13\2\2\2;:\3\2\2\2<?\3\2\2\2=>\3"+
		"\2\2\2=;\3\2\2\2>@\3\2\2\2?=\3\2\2\2@A\7/\2\2AB\7/\2\2BC\7@\2\2C\6\3\2"+
		"\2\2DE\7>\2\2EF\7#\2\2FG\7]\2\2GH\7E\2\2HI\7F\2\2IJ\7C\2\2JK\7V\2\2KL"+
		"\7C\2\2LM\7]\2\2MQ\3\2\2\2NP\13\2\2\2ON\3\2\2\2PS\3\2\2\2QR\3\2\2\2QO"+
		"\3\2\2\2RT\3\2\2\2SQ\3\2\2\2TU\7_\2\2UV\7_\2\2VW\7@\2\2W\b\3\2\2\2XY\7"+
		">\2\2YZ\7#\2\2Z^\3\2\2\2[]\13\2\2\2\\[\3\2\2\2]`\3\2\2\2^_\3\2\2\2^\\"+
		"\3\2\2\2_a\3\2\2\2`^\3\2\2\2ab\7@\2\2bc\3\2\2\2cd\b\4\2\2d\n\3\2\2\2e"+
		"f\7(\2\2fg\5%\22\2gh\7=\2\2h\f\3\2\2\2ij\7(\2\2jk\7%\2\2km\3\2\2\2ln\5"+
		"+\25\2ml\3\2\2\2no\3\2\2\2om\3\2\2\2op\3\2\2\2pq\3\2\2\2qr\7=\2\2r\177"+
		"\3\2\2\2st\7(\2\2tu\7%\2\2uv\7z\2\2vx\3\2\2\2wy\5)\24\2xw\3\2\2\2yz\3"+
		"\2\2\2zx\3\2\2\2z{\3\2\2\2{|\3\2\2\2|}\7=\2\2}\177\3\2\2\2~i\3\2\2\2~"+
		"s\3\2\2\2\177\16\3\2\2\2\u0080\u0086\t\2\2\2\u0081\u0083\7\17\2\2\u0082"+
		"\u0081\3\2\2\2\u0082\u0083\3\2\2\2\u0083\u0084\3\2\2\2\u0084\u0086\7\f"+
		"\2\2\u0085\u0080\3\2\2\2\u0085\u0082\3\2\2\2\u0086\u0087\3\2\2\2\u0087"+
		"\u0085\3\2\2\2\u0087\u0088\3\2\2\2\u0088\20\3\2\2\2\u0089\u008a\7>\2\2"+
		"\u008a\u008b\3\2\2\2\u008b\u008c\b\b\3\2\u008c\22\3\2\2\2\u008d\u008e"+
		"\7>\2\2\u008e\u008f\7A\2\2\u008f\u0090\7z\2\2\u0090\u0091\7o\2\2\u0091"+
		"\u0092\7n\2\2\u0092\u0093\3\2\2\2\u0093\u0094\5\'\23\2\u0094\u0095\3\2"+
		"\2\2\u0095\u0096\b\t\3\2\u0096\24\3\2\2\2\u0097\u0098\7>\2\2\u0098\u0099"+
		"\7A\2\2\u0099\u009a\3\2\2\2\u009a\u009b\5%\22\2\u009b\u009c\3\2\2\2\u009c"+
		"\u009d\b\n\4\2\u009d\u009e\b\n\5\2\u009e\26\3\2\2\2\u009f\u00a1\n\3\2"+
		"\2\u00a0\u009f\3\2\2\2\u00a1\u00a2\3\2\2\2\u00a2\u00a0\3\2\2\2\u00a2\u00a3"+
		"\3\2\2\2\u00a3\30\3\2\2\2\u00a4\u00a5\7@\2\2\u00a5\u00a6\3\2\2\2\u00a6"+
		"\u00a7\b\f\6\2\u00a7\32\3\2\2\2\u00a8\u00a9\7A\2\2\u00a9\u00aa\7@\2\2"+
		"\u00aa\u00ab\3\2\2\2\u00ab\u00ac\b\r\6\2\u00ac\34\3\2\2\2\u00ad\u00ae"+
		"\7\61\2\2\u00ae\u00af\7@\2\2\u00af\u00b0\3\2\2\2\u00b0\u00b1\b\16\6\2"+
		"\u00b1\36\3\2\2\2\u00b2\u00b3\7\61\2\2\u00b3 \3\2\2\2\u00b4\u00b5\7?\2"+
		"\2\u00b5\"\3\2\2\2\u00b6\u00ba\7$\2\2\u00b7\u00b9\n\4\2\2\u00b8\u00b7"+
		"\3\2\2\2\u00b9\u00bc\3\2\2\2\u00ba\u00b8\3\2\2\2\u00ba\u00bb\3\2\2\2\u00bb"+
		"\u00bd\3\2\2\2\u00bc\u00ba\3\2\2\2\u00bd\u00c7\7$\2\2\u00be\u00c2\7)\2"+
		"\2\u00bf\u00c1\n\5\2\2\u00c0\u00bf\3\2\2\2\u00c1\u00c4\3\2\2\2\u00c2\u00c0"+
		"\3\2\2\2\u00c2\u00c3\3\2\2\2\u00c3\u00c5\3\2\2\2\u00c4\u00c2\3\2\2\2\u00c5"+
		"\u00c7\7)\2\2\u00c6\u00b6\3\2\2\2\u00c6\u00be\3\2\2\2\u00c7$\3\2\2\2\u00c8"+
		"\u00cc\5/\27\2\u00c9\u00cb\5-\26\2\u00ca\u00c9\3\2\2\2\u00cb\u00ce\3\2"+
		"\2\2\u00cc\u00ca\3\2\2\2\u00cc\u00cd\3\2\2\2\u00cd&\3\2\2\2\u00ce\u00cc"+
		"\3\2\2\2\u00cf\u00d0\t\6\2\2\u00d0\u00d1\3\2\2\2\u00d1\u00d2\b\23\2\2"+
		"\u00d2(\3\2\2\2\u00d3\u00d4\t\7\2\2\u00d4*\3\2\2\2\u00d5\u00d6\t\b\2\2"+
		"\u00d6,\3\2\2\2\u00d7\u00dc\5/\27\2\u00d8\u00dc\t\t\2\2\u00d9\u00dc\5"+
		"+\25\2\u00da\u00dc\t\n\2\2\u00db\u00d7\3\2\2\2\u00db\u00d8\3\2\2\2\u00db"+
		"\u00d9\3\2\2\2\u00db\u00da\3\2\2\2\u00dc.\3\2\2\2\u00dd\u00df\t\13\2\2"+
		"\u00de\u00dd\3\2\2\2\u00df\60\3\2\2\2\u00e0\u00e1\7A\2\2\u00e1\u00e2\7"+
		"@\2\2\u00e2\u00e3\3\2\2\2\u00e3\u00e4\b\30\6\2\u00e4\62\3\2\2\2\u00e5"+
		"\u00e6\13\2\2\2\u00e6\u00e7\3\2\2\2\u00e7\u00e8\b\31\4\2\u00e8\64\3\2"+
		"\2\2\25\2\3\4=Q^oz~\u0082\u0085\u0087\u00a2\u00ba\u00c2\u00c6\u00cc\u00db"+
		"\u00de\7\b\2\2\7\3\2\5\2\2\7\4\2\6\2\2";
	public static final ATN _ATN =
		new ATNDeserializer().deserialize(_serializedATN.toCharArray());
	static {
		_decisionToDFA = new DFA[_ATN.getNumberOfDecisions()];
		for (int i = 0; i < _ATN.getNumberOfDecisions(); i++) {
			_decisionToDFA[i] = new DFA(_ATN.getDecisionState(i), i);
		}
	}
}