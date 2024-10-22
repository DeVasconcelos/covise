//-*-Mode: C++;-*-
/*

  Parser for Ensight case files

*/

// the includes
%header{
// standard includes
#include "CaseFile.h"
#include "DataItem.h"
#include <string>
#include <iostream>
#include <fstream>
#ifndef _WIN32
#include <sys/stat.h>
#endif




// prototypes
class CaseLexer;

//
// in order to avoid strange problems...
//
#define register

// define a constant for the maximum length of a string token
#define MaxTokenLength 1024
#define YYDEBUG 9
// trim from left
inline std::string& ltrim(std::string& s, const char* t = " \t\n\r\f\v")
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from right
inline std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v")
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from left & right
inline std::string& trim(std::string& s, const char* t = " \t\n\r\f\v")
{
    return ltrim(rtrim(s, t), t);
}
%}

%name CaseParser
%define USE_CONST_TOKEN 1

%define CONSTRUCTOR_PARAM const std::string &sFileName
%define CONSTRUCTOR_INIT : inputFile_( NULL ), lexer_( NULL )
%define CONSTRUCTOR_CODE init( sFileName );

%define MEMBERS \
  public:   virtual   ~CaseParser(); \
  public:   void      yyerror( const char *msg ); \
  public:   void      yyerror( const std::string &sMsg ); \
  public:   void      setCaseObj(CaseFile &cs); \
  public:   CaseFile  getCaseObj(); \
  public:   bool      isOpen(); \
  private:  std::ifstream  *inputFile_; \
  private:  CaseLexer *lexer_; \
  private:  CaseFile  caseFile_; \
  private:  DataItem  *actIt_;\
  private:  TimeSet   *actTs_;\
  private:  bool      isOpen_;\
  private:  int       tsStart_; \
  private:  int init( const std::string &sFileName ); \


%union 
{  
    struct {
	char szValue[ MaxTokenLength ];
	int iVal;  
	double dVal;  
    } token;
    
}



%header{
  // Give Bison-tokentType a readable name
  typedef YY_CaseParser_STYPE MyTokenType;

%}

// work with untyped tokens


%token FORMAT_SEC TYPE GEOMETRY_SEC MODEL MEASURED MATCH CH_CO_ONLY VARIABLE_SEC CONSTANT COMPLEX 
%token SCALAR VECTOR PER_CASE PER_NODE PER_ELEMENT TIME_SEC TIME_SET NUM_OF_STEPS 
%token FN_ST_NUM FN_INCR 
%token TIME_VAL IDENTIFIER POINT_IDENTIFIER INTEGER DOUBLE STRING IPADDRESS
%token VARDESC TENSOR_SYMM
%token ENSIGHTV ENSIGHT_GOLD ASTNOTFN FN_NUMS FLOAT
%token PER_M_NODE PER_M_ELEMENT
%token VAR_POST VAR_POST_TS VAR_INT
%token FILE_SEC FILE_SET

%left LOGICAL_OR 
%left LOGICAL_AND 

%left '+' '-' 
%left '/' '*' 
%left U_SUB

%%

// 


ecase: section 
     | ecase section

section: sect_key  spec

sect_key: FORMAT_SEC 
        | GEOMETRY_SEC
        | VARIABLE_SEC
        | TIME_SEC
        | FILE_SEC

spec: spec_line
    | spec spec_line


spec_line: type_spec 
         | model_spec 
         | variable_spec
         | ts_spec
         | fs_spec
         | const_spec


ts_spec: ts_hdr ts_opts
         {
	     	     fprintf(stderr, "ts_hdr ts_opts %s %s\n",$<token>1.szValue, $<token>2.szValue);
         }
        |ts_spec ts_hdr ts_opts
         {
	     	     fprintf(stderr, "ts_hdr ts_opts %s %s\n",$<token>1.szValue, $<token>2.szValue);
         }

ts_hdr: TIME_SET INTEGER NUM_OF_STEPS INTEGER 
         {
	     int ts( $<token>2.iVal );
	     int ns( $<token>4.iVal );
	     	     fprintf(stderr, "DEFINITION TIMESET %d  STEPS %d\n", ts, ns);
	     actTs_ = new TimeSet( ts, ns );

	 }
         | TIME_SET INTEGER IDENTIFIER NUM_OF_STEPS INTEGER 
         {
	     int ts( $<token>2.iVal );
	     int ns( $<token>5.iVal );
	          fprintf(stderr, "DEFINITION TIMESET %d  STEPS %d\n", ts, ns);
	     actTs_ = new TimeSet( ts, ns );

	 } 

ts_opts: ts_fnum_sec ts_tval_secc
         | ts_tval_secc
         | ts_fnum_sec
         | ts_fn_start ts_fn_incr ts_tval_secc


ts_fn_start: FN_ST_NUM INTEGER
         {
	     int fs( $<token>2.iVal );
	     tsStart_ = fs;
	     	     fprintf(stderr, " FIELNAME START %d ", fs);
	 }

ts_fn_incr: FN_INCR INTEGER
         {
	     int incr( $<token>2.iVal ); 
	     	     fprintf(stderr, " FIELNAME INCREMENT %d ", incr);
	     if (actTs_ != NULL) {
		 int i;
		 int j(0);
		 for ( i=tsStart_; j<actTs_->getNumTs(); i+=incr) {
		     actTs_->addFileNr(i);
		     ++j;
		 }
		 if ( actTs_->getNumTs() == actTs_->size() ) {
		     // the time-set is full
		     caseFile_.addTimeSet( actTs_ );
		     actTs_ = NULL;		 
		 }

	     }

	 }

ts_fnum_sec: FN_NUMS ts_fn_nums 
         {
	     	     fprintf(stderr, " FILENAME NUMBERS ");
	 }

ts_fn_nums: ts_fn_nums INTEGER
         {
		 
	     	     fprintf(stderr, " INTEGER ");
	     int nf( $<token>2.iVal ); 
	     if (actTs_ != NULL) {
		 actTs_->addFileNr(nf); 		 	     
		 if ( actTs_->getNumTs() == actTs_->size() ) {
		     // the time-set is full
		     caseFile_.addTimeSet( actTs_ );
		     actTs_ = NULL;		 
		 }
	     }
	 }
         |  INTEGER
         {
	     	     fprintf(stderr, " INTEGER ");
	     int nf( $<token>1.iVal ); 
	     //	     fprintf(stderr, " %d ", nf);
	     if (actTs_ != NULL) {
		 actTs_->addFileNr(nf); 
		 if ( actTs_->getNumTs() == actTs_->size() ) {
		     // the time-set is full
		     caseFile_.addTimeSet( actTs_ );
		     actTs_ = NULL;		 
		 }
	     }
	 }

ts_tval_secc: TIME_VAL ts_tvals
         {
	     	     fprintf(stderr, " TIME_VAL ts_tvals ");
	     if ( actTs_ != NULL ) {
	     }
	 }

ts_tvals: ts_tvals DOUBLE
         {
	     	     fprintf(stderr, " ts_tvals DOUBLE ");
	     float rt( (float) $<token>2.dVal );
	     if (actTs_ != NULL) {
		 actTs_->addRealTimeVal(rt); 
	     }
	     else {
		 actTs_ = caseFile_.getLastTimeSet();
		 actTs_->addRealTimeVal(rt); 
	     }
	 }
	 | ts_tvals INTEGER
         {
	     	     fprintf(stderr, " ts_tvals INTEGER ");
	     float rt( (float) $<token>2.iVal );
	     if (actTs_ != NULL) {
		 actTs_->addRealTimeVal(rt); 
	     }
	     else {
		 actTs_ = caseFile_.getLastTimeSet();
		 actTs_->addRealTimeVal(rt); 
	     }
	 }

        | DOUBLE
         {
	     	     fprintf(stderr, " DOUBLE ");
	     float rt( (float)$<token>1.dVal );
	     if (actTs_ != NULL) {
		 actTs_->addRealTimeVal(rt); 
	     }
	     else {
		 actTs_ = caseFile_.getLastTimeSet();
		 actTs_->addRealTimeVal(rt); 
	     }
	 }
        | INTEGER
         {
	     	     fprintf(stderr, " INTEGER ");
	     float rt( (float)$<token>1.iVal );
	     if (actTs_ != NULL) {
		 actTs_->addRealTimeVal(rt); 
	     }
	     else {
		 actTs_ = caseFile_.getLastTimeSet();
		 actTs_->addRealTimeVal(rt); 
	     }
	 }
	 
	 

fs_spec: fs_hdr
        |fs_spec fs_hdr
		
fs_hdr: FILE_SET INTEGER NUM_OF_STEPS INTEGER 
         {
	     int ts( $<token>2.iVal );
	     int ns( $<token>4.iVal );
	     	     fprintf(stderr, "DEFINITION FILESET %d  STEPS %d\n", ts, ns);
	     //actTs_ = new TimeSet( ts, ns );

	 }
         | FILE_SET INTEGER IDENTIFIER NUM_OF_STEPS INTEGER 
         {
	     int ts( $<token>2.iVal );
	     int ns( $<token>5.iVal );
	     	     fprintf(stderr, "DEFINITION FILESET %d  STEPS %d\n", ts, ns);
	     //actTs_ = new TimeSet( ts, ns );

	 } 


type_spec: TYPE ENSIGHTV
    {
	//	fprintf(stderr,"  ENSIGHT VERSION 6 found\n");
	caseFile_.setVersion(CaseFile::v6);
    }
    | TYPE ENSIGHTV ENSIGHT_GOLD
    {
	//	fprintf(stderr,"  ENSIGHT GOLD found\n");
	caseFile_.setVersion(CaseFile::gold);
    }


any_identifier: IDENTIFIER
    | POINT_IDENTIFIER
    | STRING

model_spec: MODEL any_identifier 
          {
        	std::string ensight_geofile($<token>2.szValue);
		//	        fprintf(stderr,"  ENSIGHT MODEL <%s> found\n", ensight_geofile.c_str());
	        caseFile_.setGeoFileNm( ensight_geofile );
          }
          | MODEL INTEGER any_identifier 
          {
	      std::string ensight_geofile($<token>3.szValue);
          // check whether the integer is part of the filename
	  // this integer is separated by a whitespace from the filename thus no need to add it to the filename
	  // opening the file never works if you are not in the same directory as the case file
	  // INTEGER now requires a whitespace, see lexer
          /*struct stat buf;
          stat( ensight_geofile.c_str(), &buf);
          if( !S_ISREG(buf.st_mode) )
          {
             std::string intStr($<token>2.szValue); 
	         caseFile_.setGeoFileNm( intStr+ensight_geofile );
          }
          else
          {*/
	      caseFile_.setGeoFileNm( ensight_geofile );
	      int ts( $<token>2.iVal );
	      caseFile_.setGeoTsIdx(ts);
          //}   
	      	      fprintf(stderr,"  ENSIGHT MODEL <%s> TIMESET <%d> found\n", ensight_geofile.c_str(), ts);
          }
          | MODEL INTEGER INTEGER any_identifier 
          {
	      std::string ensight_geofile($<token>4.szValue);
	      caseFile_.setGeoFileNm( ensight_geofile );
	      int ts( $<token>2.iVal );
	      caseFile_.setGeoTsIdx(ts);
	      //	      fprintf(stderr,"  ENSIGHT MODEL <%s> TIMESET <%d> found\n", ensight_geofile.c_str(), ts);
          }


          | MODEL INTEGER ASTNOTFN 
          {
	      std::string ensight_geofile($<token>3.szValue);
	      caseFile_.setGeoFileNm( ensight_geofile );
	      int ts( $<token>2.iVal );
	      caseFile_.setGeoTsIdx(ts);
	      //	      fprintf(stderr,"  ENSIGHT MODEL <%s> TIMESET <%d> found\n", ensight_geofile.c_str(), ts);
          }
          // we may find lines like: model bla_geo.**** in this case we set the timeset to 1 
          | MODEL ASTNOTFN 
          {	      
	      std::string ensight_geofile($<token>2.szValue);
	      caseFile_.setGeoFileNm( ensight_geofile );
	      caseFile_.setGeoTsIdx(1);
	      //	      fprintf(stderr,"  ENSIGHT MODEL <%s> TIMESET <%d> found\n", ensight_geofile.c_str(), ts);
          }


          | MEASURED any_identifier 
          {
        	std::string ensight_geofile($<token>2.szValue);
	        caseFile_.setMGeoFileNm( ensight_geofile );
          }
          | MEASURED INTEGER any_identifier 
          {
	      std::string ensight_geofile($<token>3.szValue);
	      caseFile_.setMGeoFileNm( ensight_geofile );
	      int ts( $<token>2.iVal );
	      caseFile_.setGeoTsIdx(ts);
          }
          | MEASURED INTEGER INTEGER any_identifier 
          {
	      std::string ensight_geofile($<token>4.szValue);
	      caseFile_.setMGeoFileNm( ensight_geofile );
	      int ts( $<token>2.iVal );
	      caseFile_.setGeoTsIdx(ts);
          }
          | MEASURED INTEGER ASTNOTFN 
          {
	      std::string ensight_geofile($<token>3.szValue);
	      caseFile_.setMGeoFileNm( ensight_geofile );
	      int ts( $<token>2.iVal );
	      caseFile_.setGeoTsIdx(ts);

	      //	      fprintf(stderr,"  ENSIGHT MEASURED <%s> TIMESET <%d> found\n", ensight_geofile.c_str(), ts);
          }
          // we may find lines like: model bla_geo.**** in this case we set the timeset to 1 
          | MEASURED ASTNOTFN 
          {	      
	      std::string ensight_geofile($<token>2.szValue);
	      caseFile_.setMGeoFileNm( ensight_geofile );
	      caseFile_.setGeoTsIdx(1);
          }

        

variable_spec: var_pre IDENTIFIER STRING
          {
	      	      fprintf(stderr," var_pre STRING %s\n", $<token>2.szValue);
	      std::string tmp($<token>2.szValue);
	      size_t last = tmp.find(" ");
	      if ( last == std::string::npos ) {
		  last = tmp.find("\t");
		  if ( last == std::string::npos ) {
		      std::cerr << "CaseParser::yyparse() filename or description for variable missing" << std::endl;
		  }
	      }
	      std::string desc( tmp.substr( 0, last ) );
	      size_t len( tmp.size() );
	      size_t snd( tmp.find_first_not_of(" ",last) );
	      std::string fname( tmp.substr( snd, len-snd ) );
	      //	      fprintf(stderr," VAR_POST DE<%s>   FN<%s>\n", desc.c_str(), fname.c_str() );
	      
	      actIt_->setDesc( desc );

	      actIt_->setFileName( trim(fname) );
	      
	      if ( actIt_ != NULL ) {
		  caseFile_.addDataIt( *actIt_ );
	      }
	      else {
		  std::cerr << "CaseParser::yyparse() try to add NULL DataItem" << std::endl;
	      }

	      delete actIt_;
	      actIt_ = NULL;

	  }
	  | var_pre  VAR_POST
          {
	      	      fprintf(stderr," VAR_POST %s\n", $<token>2.szValue);
	      std::string tmp($<token>2.szValue);
	      size_t last = tmp.find(" ");
	      if ( last == std::string::npos ) {
		  last = tmp.find("\t");
		  if ( last == std::string::npos ) {
		      std::cerr << "CaseParser::yyparse() filename or description for variable missing" << std::endl;
		  }
	      }
	      std::string desc( tmp.substr( 0, last ) );
	      size_t len( tmp.size() );
	      size_t snd( tmp.find_first_not_of(" ",last) );
	      std::string fname( tmp.substr( snd, len-snd ) );
	      //	      fprintf(stderr," VAR_POST DE<%s>   FN<%s>\n", desc.c_str(), fname.c_str() );
	      
	      actIt_->setDesc( desc );

	      actIt_->setFileName( trim(fname) );
	      
	      if ( actIt_ != NULL ) {
		  caseFile_.addDataIt( *actIt_ );
	      }
	      else {
		  std::cerr << "CaseParser::yyparse() try to add NULL DataItem" << std::endl;
	      }

	      delete actIt_;
	      actIt_ = NULL;

	  }
          | var_pre VAR_INT VAR_POST
          {
	      	      fprintf(stderr," VAR_INT VAR_POST_TS %s\n", $<token>3.szValue);
	      std::string tmp($<token>3.szValue);
	      size_t last = tmp.find(" ");
	      if ( last == std::string::npos ) {
		  last = tmp.find("\t");
		  if ( last == std::string::npos ) {
		      std::cerr << "CaseParser::yyparse() filename or description for variable missing" << std::endl;
		  }
	      }
	      std::string desc( tmp.substr( 0, last ) );
	      size_t len( tmp.size() );
	      size_t snd( tmp.find_first_not_of(" ",last) );
	      std::string fname( tmp.substr( snd, len-snd ) );
	      //	      fprintf(stderr," VAR_POST DE<%s>   FN<%s>\n", desc.c_str(), fname.c_str() );
	      
	      actIt_->setDesc( desc );

	      actIt_->setFileName( trim(fname) );
	      
	      if ( actIt_ != NULL ) {
		  caseFile_.addDataIt( *actIt_ );
	      }
	      else {
		  std::cerr << "CaseParser::yyparse() try to add NULL DataItem" << std::endl;
	      }

	      delete actIt_;
	      actIt_ = NULL;

	  }
          | var_pre VAR_INT VAR_INT VAR_POST
          {
	      	      fprintf(stderr," VAR_INT VAR_POST_TS %s\n", $<token>4.szValue);
	      std::string tmp($<token>4.szValue);
	      size_t last = tmp.find(" ");
	      if ( last == std::string::npos ) {
		  last = tmp.find("\t");
		  if ( last == std::string::npos ) {
		      std::cerr << "CaseParser::yyparse() filename or description for variable missing" << std::endl;
		  }
	      }
	      std::string desc( tmp.substr( 0, last ) );
	      size_t len( tmp.size() );
	      size_t snd( tmp.find_first_not_of(" ",last) );
	      std::string fname( tmp.substr( snd, len-snd ) );
	      //	      fprintf(stderr," VAR_POST DE<%s>   FN<%s>\n", desc.c_str(), fname.c_str() );
	      
	      actIt_->setDesc( desc );

	      actIt_->setFileName( trim(fname) );
	      
	      if ( actIt_ != NULL ) {
		  caseFile_.addDataIt( *actIt_ );
	      }
	      else {
		  std::cerr << "CaseParser::yyparse() try to add NULL DataItem" << std::endl;
	      }

	      delete actIt_;
	      actIt_ = NULL;

	  }
          | var_pre IDENTIFIER DOUBLE
          {
	      	      fprintf(stderr," var_type var_rela %s\n", $<token>3.szValue);
	  }

var_pre :  var_type var_rela 
          {
	      	      fprintf(stderr,"var_pre: var_type var_rela\n");
	  }

var_type: SCALAR
          {  
	      actIt_ = new DataItem;
	      	      fprintf(stderr,"     ENSIGHT SCALAR VARIABLE \n");
	      actIt_->setType( DataItem::scalar );
	  }
          | VECTOR 
          {  
	      actIt_ = new DataItem;
	      	      fprintf(stderr,"     ENSIGHT VECTOR VARIABLE ");
	      actIt_->setType( DataItem::vector );
	  }
          | TENSOR_SYMM 
          {  
	      actIt_ = new DataItem;
	      	      fprintf(stderr,"     ENSIGHT SYMMETRIC TENSOR VARIABLE ");
	      actIt_->setType( DataItem::tensor );
	  }


var_rela: PER_ELEMENT 
          {  
	      actIt_->setDataType(false);
	      actIt_->setMeasured(false);
	      	      fprintf(stderr," PER ELEMENT DATA");
	  }
          | PER_NODE
          {  
	      actIt_->setDataType(true);
	      actIt_->setMeasured(false);
	      	      fprintf(stderr," PER NODE DATA");
	  }
          | PER_M_NODE
          {  
	      actIt_->setDataType(true);
	      actIt_->setMeasured(true);
	      	      fprintf(stderr," PER NODE DATA");
	  }
          | PER_M_ELEMENT
          {  
	      actIt_->setDataType(false);
	      actIt_->setMeasured(true);
	      	      fprintf(stderr," PER NODE DATA");
	  }
          | PER_CASE
          {  
	      	      fprintf(stderr," PER CASE ");
	  }

const_spec: CONSTANT var_rela POINT_IDENTIFIER INTEGER
          | CONSTANT var_rela POINT_IDENTIFIER DOUBLE
          | CONSTANT var_rela IDENTIFIER DOUBLE
          | CONSTANT var_rela IDENTIFIER INTEGER
          | CONSTANT var_rela INTEGER POINT_IDENTIFIER INTEGER
          | CONSTANT var_rela INTEGER POINT_IDENTIFIER DOUBLE
          | CONSTANT var_rela INTEGER IDENTIFIER INTEGER
          | CONSTANT var_rela INTEGER IDENTIFIER DOUBLE

               


%%
// end of rule definition
#include "CaseLexer.h"
#include "CaseFile.h"
#include "DataItem.h"


int CaseParser::init( const std::string &sFileName)
{
    isOpen_ = false;

    inputFile_ = new std::ifstream( sFileName.c_str() );
    if (!inputFile_->is_open()) {
        fprintf(stderr,"could not open %s for reading",sFileName.c_str());
        delete inputFile_;
        inputFile_ = NULL;
        return( 0 );
    }
    inputFile_->peek(); // try to exclude directories
    if( inputFile_->fail() ) {
        fprintf(stderr,"could not open %s for reading - fail",sFileName.c_str());
        delete inputFile_;
        inputFile_ = NULL;
        return( 0 );
    }

    isOpen_ = true;
  
    lexer_ = new CaseLexer( (istream *) inputFile_ );
    lexer_->set_debug( 1 );
    //lexer_->set_debug( 0 );

    actIt_ = NULL;
    actTs_ = NULL;

    return( 0 );
}

CaseParser::~CaseParser()
{
    if( lexer_ ) {
        delete lexer_; lexer_ = NULL;
    }

    if( inputFile_ && (*inputFile_) ) {
        inputFile_->close();
        delete inputFile_;
        inputFile_ = NULL;
    }
}

// The method yylex must be defined to work with flex
int 
CaseParser::yylex()
{
    return( lexer_->scan( &yylval ) );
}

void 
CaseParser::yyerror( char *szErrMsg )
{
  //    std::cerr << std::endl << szErrMsg << "(line " << lexer_->lineno() << ")";
    fprintf(stderr,"%s (line: %d )\n",szErrMsg,lexer_->lineno());
}

void 
CaseParser::yyerror( const char *szErrMsg )
{
    yyerror( const_cast<char *>(szErrMsg) );
}

void 
CaseParser::yyerror( const std::string &sMsg )
{
    yyerror( sMsg.c_str() );
}

void 
CaseParser::setCaseObj(CaseFile &cs)
{
    caseFile_ = cs;
}

CaseFile
CaseParser::getCaseObj()
{
    return caseFile_;
}

bool
CaseParser::isOpen()
{
    return isOpen_;
}



