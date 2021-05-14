
"use strict" ;


// Require the lib, get a working terminal
var termkit = require( 'terminal-kit' );
var term    = termkit.terminal ;
const Promise = require( 'seventh' ) ;


term.clear();
term.hideCursor( true ) ;


function terminate() {
	term.grabInput( false ) ;
  term.styleReset() ; 
  term.clear() ;
  term( 'We are closing down now.!\n' ) ;
	setTimeout( function() {term.hideCursor( false ) ;process.exit() } , 100 ) ;
}



var document = term.createDocument( {
palette: new termkit.Palette()
	// backgroundAttr: { bgColor: 'magenta' , dim: true } ,
} ) ;



term.clear() ;

var document = term.createDocument( {
//	palette: new termkit.Palette()
	//	backgroundAttr: { bgColor: 'magenta' , dim: true } ,
} ) ;

var layout = new termkit.Layout( {
	parent: document ,
	boxChars: 'double' ,
	layout: {
		id: 'main' ,
		y: 1 , x: 0,
		widthPercent: 100 ,
    heightPercent: 100 ,
		rows: [
			{
				columns: [
					{ id: 'users' } ,
					{ id: 'message' , widthPercent: 100/3 } 
				]
			} ,
			{
			  height: 3 ,
				columns: [
					{ id: 'footer' 
          } ,
				]
			}
		]
	}
} ) ;




var textBox = new termkit.TextBox( {
	parent: document.elements.message ,
	//content: text ,
  contentHasMarkup: true ,
	scrollable: true ,
	vScrollBar: true ,
	lineWrap: true ,
	 wordWrap: false ,
	x: 1 ,
	y: 0 ,
	autoWidth: 1 ,
	autoHeight: 1
} ) ;

/*

var window = new termkit.Window( {
	parent: document ,
	//boxChars: 'dotted' ,
	x: 50 ,
	y: 10 ,
	width: 50 ,
	height: 10 ,
	title: "^c^+Cool^:, a ^/window^:!" ,
	titleHasMarkup: true ,
	movable: true ,
	scrollable: true ,
	vScrollBar: true ,
	//hScrollBar: true ,

	// Features that are planned, but not yet supported:
	minimizable: true ,
	dockable: true ,
	closable: true ,
	resizable: true
} ) ;

var content = [
	'This is the window content...' ,
	'Second line of content...' ,
	'Third line of content...'
] ;

for ( let i = 4 ; i <= 30 ; i ++ ) { content.push( '' + i + 'th line of content...' ) ; }

new termkit.Text( {
	parent: window ,
	content ,
	attr: { color: 'green' , italic: true }
} ) ;

*/

async function randomLogs() {
	var index = 0 ,
		randomStr = [
			"[INFO] Initilizing..." ,
			"[INFO] Exiting..." ,
			"[INFO] Reloading..." ,
			"[WARN] No config found" ,
			"[VERB] Client disconnected" ,
			"[INFO] Loading data" ,
			"[VERB] Awesome computing in progress" ,
			"[VERB] Lorem ipsum"
		] ;

	while ( true ) {
		await Promise.resolveTimeout( 50 + Math.random() * 1000 ) ;
		textBox.appendLog( "Log #" + ( index ++ ) + ' ' + randomStr[ Math.floor( Math.random() * randomStr.length ) ] ) ;
	}
}

randomLogs() ;






/*

var textTable = new termkit.TextTable( {
	parent: document ,
	cellContents: [
		//*
		[ 'header #1' , 'header #2' , 'header #3' ] ,
		[ 'row #1' , 'a much bigger cell '.repeat( 10 ) , 'cell' ] ,
		[ 'row #2' , 'cell' , 'a medium cell' ] ,
		[ 'row #3' , 'cell' , 'cell' ] ,
		[ 'row #4' , 'cell\nwith\nnew\nlines' , 'cell' ]
		//*/
		/*
		[ '1-1' , '2-1' , '3-1' ] ,
		[ '1-2' , '2-2' , '3-2' ] ,
		[ '1-3' , '2-3' , '3-3' ]
		//* /
	] ,
	contentHasMarkup: true ,
	x: 0 ,
	y: 2 ,
	//hasBorder: false ,
	//borderChars: 'double' ,
	borderAttr: { color: 'blue' } ,
	textAttr: { bgColor: 'default' } ,
	//textAttr: { bgColor: 'black' } ,
	firstCellTextAttr: { bgColor: 'blue' } ,
	firstRowTextAttr: { bgColor: 'gray' } ,
	firstColumnTextAttr: { bgColor: 'red' } ,
	checkerEvenCellTextAttr: { bgColor: 'gray' } ,
	evenCellTextAttr: { bgColor: 'gray' } ,
	evenRowTextAttr: { bgColor: 'gray' } ,
	evenColumnTextAttr: { bgColor: 'gray' } ,
	selectedTextAttr: { bgColor: 'blue' } , selectable: 'cell' ,
	//width: 50 ,
	width: term.width ,
	height: 20 ,
	fit: true ,	// Activate all expand/shrink + wordWrap
	//expandToWidth: true , shrinkToWidth: true , expandToHeight: true , shrinkToHeight: true , wordWrap: true ,
	//lineWrap: true ,
} ) ;


setTimeout( () => {
	textTable.setCellContent( 2 , 3 , "New ^R^+content^:! And BTW... We have to force some line break and so on..." ) ;
} , 1000 ) ;
//*/
/*
setTimeout( () => {
	textTable.setCellAttr( 1 , 2 , { bgColor: 'cyan' } ) ;
	textTable.setRowAttr( 2 , { bgColor: 'cyan' } ) ;
	textTable.setColumnAttr( 1 , { bgColor: 'cyan' } ) ;
	textTable.setTableAttr( { bgColor: 'cyan' } ) ;
} , 500 ) ;

setTimeout( () => {
	textTable.resetCellAttr( 1 , 2 ) ;
	textTable.resetRowAttr( 2 ) ;
	textTable.resetColumnAttr( 1 ) ;
	textTable.resetTableAttr() ;
} , 1500 ) ;

*/



/*
*/

term.hideCursor() ;
//layout.draw() ;
//layout.setAutoResize( true ) ;


var header = new termkit.Text( {
	parent: document,
	content: 'Now Talk Switchboard Server manager '+ term.height +" "+ term.width,
  autoWidth: 1,
	attr: {
		color: 'White' ,
    bgColor: 'blue',
		bold: true ,
		italic: true
	}
} ) ;

var footer = new termkit.Text( {
	parent: document.elements.footer,
	content: "Responsive terminal layout! Try resizing your terminal! ;)" ,
  autoWidth: 100 ,
	attr: {
		color: 'brightMagenta' ,
		bold: true ,
		italic: true
	}
} ) ;

//term.moveTo( 1 , term.height ) ;

/*




var window = new termkit.Window( {
	parent: document ,
	//boxChars: 'dotted' ,
	x: 10 ,
	y: 10 ,
	width: 50 ,
	height: 10 ,
	inputHeight: 30 ,
	title: "^c^+Cool^:, a ^/window^:!" ,
	titleHasMarkup: true ,
	movable: true ,
	scrollable: true ,
	vScrollBar: true ,
	//hScrollBar: true ,

	// Features that are planned, but not yet supported:
	minimizable: true ,
	dockable: true ,
	closable: true ,
	resizable: true
} ) ;

var content = [
	'This is the window content...' ,
	'Second line of content...' ,
	'Third line of content...'
] ;

for ( let i = 4 ; i <= 30 ; i ++ ) { content.push( '' + i + 'th line of content...' ) ; }

new termkit.Text( {
	parent: window ,
	content ,
	attr: { color: 'green' , italic: true }
} ) ;

term.moveTo( 1 , 1 ) ;
*/



term.on( 'key' , function( key  , matches , data ) {
	switch( key ) {
		case 'CTRL_C' :
			terminate();
			break ;
   }
} ) ;
