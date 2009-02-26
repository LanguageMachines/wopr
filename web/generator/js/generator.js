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
	  var parser = new DOMParser();
	  var str = "<response><data>Server not running.</data></response>";
	  ans = parser.parseFromString(str,'text/xml');
	}
	
	var new_html = "";
	var items = ans.documentElement.getElementsByTagName("data");
	for (var i = 0; i < items.length; i++) {
	  var data = items[i].childNodes[0].nodeValue;
	  new_html = new_html + data + "<br />";
	}
	output_el.set('html', new_html);
      },
	onFailure:function(responseText, responseXML) {
	alert("Error.");
      }
    });
  
  req.send('h=localhost&p=1984');
}
//
//]]>
