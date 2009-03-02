//<![CDATA[
//
// (c) Peter Berck, ILK, 2009
//
function generate_one() {
  var generator_url = "./generator.php";
  var output_el     = $('output');

  var req = new Request( {url:generator_url,
	onSuccess:function(responseText, responseXML) {
	ans = responseXML;

	if ( ! ans ) {
	  output_el.set('html', "Server is not running...");
	  return;
	}
	
	var new_html = "";
	var sentences = ans.documentElement.getElementsByTagName("sentence");
	for (var s = 0; s < sentences.length; s++) {
	  var words = sentences[s].getElementsByTagName("word");
	  new_html = new_html + "&nbsp;&nbsp;&nbsp;&nbsp;";
	  for ( var w = 0; w < words.length; w++ ) {
	    var word = words[w].childNodes[0].nodeValue;
	    var idx  = words[w].getAttribute("idx");
	    var cnt  = words[w].getAttribute("cnt");
	    var klass = "plain";
	    if ( cnt == 1 ) {
	      klass = "nochoice";
	    }
	    new_html = new_html + "<span class=\""+klass+"\">";
	    new_html = new_html + word;
	    new_html = new_html + "</span> ";
	  }
	  new_html = new_html + "<br />";
	}
	output_el.set('html', new_html);
      },
	onFailure:function(responseText, responseXML) {
	output_el.set('html', "Error, cannot get a result from server...");
      }
    });
  
  req.send('h=localhost&p=1984');
}
//
//]]>
