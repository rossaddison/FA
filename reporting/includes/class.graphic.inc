<?php
/*

$pg = new Chart('bar'); // Width and Height is set automatically4
$pg->setTitle("Testing the Chart");
$pg->setLabels(array('Jan', 'Feb', 'Mar'));
$pg->addSerie('Sales', array(10345, 15666, 12222), false, true); 
$pg->addSerie('Costs', array(20767, 10456, 2888), false, false);
$pg->addSerie('Result', array(15657, 7567, 12890), 'spline', false);
$pg->setXTitle("Names");
$pg->setYTitle("Amount");
$pg->setDTitle(number_format("15666")); // only printed on donut or half donut
//$filename = "test.png";
$pg->display(); // with filename to file

In your html file you set it up as:
.....
<img src="example.php" border="1" />
.....

You can supply extra parameters to display(). Ex. $pg->display("test.png") will save the image to a file.
Ex. $pg->display("", true) will paint a border around the image. It might be suitable if you choose to save to
file for later presentation.

///// END OF EXAMPLE.PHP /////

	Here is a list of all types you may set:

	type
	----
	'bar'			1 => Vertical bars (default)
	'horizontalBar' 2 => Horizontal bars
	'dot'			3 => Dots
	'line'			4 => Lines - Series Type Fill => Area
	'pie'			5 => Pie
	'donut'			6 => Donut
	'halfDonut'		7 => Half Donut
	'spline'		8 => Splines - Series Type fill => Area
	'table'			9 => Simple HTML table

	labels[0]  =>  Name of the first parameter in Axis X
	labels[1]  =>  Name of the second parameter in Axis X
	... (etc)

	series[0]['data'][0]  =>  Value relative for "labels[0]"
	series[0]['data'][1]  =>  Value relative for "labels[1]"
	... (etc)

	series[1]['data'][0]  =>  Value relative for "labels[0]"
	series[1]['data'][1]  =>  Value relative for "labels[1]"
	... (etc)
   
	NOTE: When using 'pie', 'donut' and 'halfDonut' only the first data set is used.
   
	That's all! Hope you make a good use of it!
	It would be nice to receive feedback from others users. All comments are welcome!

	Regards,

	Joe Hunt
*/
define('MAXLEN', 27); // we cut after 25 chars + ...
class Chart
{
	/** @var string */
	var $id;
	/** @var array<int, string> */
	var $labels = array();
	/** @var array<int, array<string, mixed>> */
	var $series = array();

	/**
	 * @var string[]
	 */
	static $palette = array('#008cc9','#4db625','#ef5500','#eef100','#05c6e6', '#5ee66a'); // Currently in Use	
	/** @var array<string, int|string|false> */
	var $color = array();
	/** @var int */
	var $num_series = 0;
	/** @var string|null */
	var $title;
	/** @var string|null */
	var $axis_x;
	/** @var string|null */
	var $axis_y;

	/**
	 * @var int|numeric
	 */
	var $type = 1;
	/** @var string */
	var $donut_title = "";
	/** @var bool */
	var $set_values = true;

	/**
	 * @var int
	 */
	var $angle = 0;

	/**
	 * @var false
	 */
	var $latin_notation;
	/** @var int|float */
	var $width;
	/** @var int|float */
	var $height;
	/** @var int|float */
	var $title_height;
	/** @var int */
	var $size = 12;
	/** @var int */
	var $tsize = 18;

	/**
	 * @var int
	 */
	var $num_labels;
	/** @var array<int, int|float> */
	var $sum_total = array();
	/** @var int|float */
	var $higher_value_size = 0;
	/** @var int|float|string|null */
	var $max_value;
	/** @var string */
	var $max_label;
	/** @var string */
	var $max_legend_str;
	/** @var int|float */
	var $legend_box_height;
	/** @var string|null */
	var $stream = 'svg'; // stream = 'svg', 'png', 'jpg' or 'gif'

	/**
	 * @var SVG|null
	 */
	var $svg; // The SVG engine class.

	/**
	 * @var GdImage|false|null
	 */
	var $img; // png image

	/**
	 * @var int
	 */
	var $dec1 = 0;

	/**
	 * @var int
	 */
	/** @var int */
	var $dec2 = 0;

	/**
	 * @var string
	 */
	/** @var string */
	var $fontfile = "";

	/**
	 * @var string
	 */
	/** @var string */
	var $encoding;
	/** @var string */
	var $out_dir = "";
	/** @var string */
	var $dir = "ltr";

	/**
	 * @var string
	 */
	/** @var string */
	var $align;
	/** @var string */
	var $lang;

	/**
	 * @var string
	 */
	/** @var string */
	var $font;

	/**
	 * @var string
	 */
	/** @var string */
	var $path;

	/**
	 * @var int
	 */
	/** @var int|float */
	var $bar_width = 0;

	/**
	 * @var int
	 */
	/** @var int|float */
	var $space_between_dots	= 0;

	/**
	 * @var float|int
	 */
	/** @var int|float */
	var $higher_value = 0;
	/** @var int|float|string|null */
	var $higher_value_str = null;
	/** @var int|float */
	var $graphic_area_width	= 0;
	/** @var int|float */
	var $graphic_area_height = 0;
	/** @var int|float */
	var $graphic_area_x1 = 0;
	/** @var int|float */
	var $graphic_area_x2 = 0;
	/** @var int|float */
	var $graphic_area_y1 = 0;
	/** @var int|float */
	var $graphic_area_y2 = 0;

	/**
	 * @var bool
	 */
	/** @var bool */
	var $legend_exists = false;
	/** @var int|float */
	var $legend_box_width = 0;
	/** @var int|float */
	var $legend_box_x1 = 0;
	/** @var int|float */
	var $legend_box_x2 = 0;
	/** @var int|float */
	var $legend_box_y1 = 0;
	/** @var int|float */
	var $legend_box_y2 = 0;


	/**
	 * @param string|null $id
	 * @param int|null $width
	 * @param int|null $height
	 */
	function __construct($type = 1, $id = 'id', $width = 0, $height = 0)
	{
		$this->encoding = strtoupper($_SESSION['language']->encoding);
		//$this->encoding = "UTF-8"; // Use alternative if used in FA
		$this->dir = $_SESSION['language']->dir;
		//$this->dir = 'ltr';
		$this->lang = $_SESSION['language']->code == 'C' ? 'en' : substr($_SESSION['language']->code, 0, 2);
		//$this->lang = 'en';
		$this->path = dirname(__FILE__).'/../fonts/';
		//$this->path = dirname(__FILE__).'/';
		// You can use another UTF-8 font and put it in config.php with the name in $UTF8_fontfile 
		if ($this->encoding == 'UTF-8' && !empty(sysprefs()->UTF8_fontfile))
			$this->fontfile = $this->path.sysprefs()->UTF8_fontfile;
		elseif ($this->dir == 'rtl')
			$this->fontfile = $this->path.'zarnormal.ttf';
		else	
			$this->fontfile = $this->path.'FreeSans.ttf';
		//$this->fontfile = $this->path.'FreeSans.ttf';
		//$this->fontfile = $this->path.'zarnormal.ttf';
		$this->id = $id;
		if ($type == 'bar') $type = 1;
		elseif ($type == 'horizontalBar') $type = 2;
		elseif ($type == 'dot') $type = 3;
		elseif ($type == 'line') $type = 4;
		elseif ($type == 'spline') $type = 8;
		elseif ($type == 'pie') $type = 5;
		elseif ($type == 'donut') $type = 6;
		elseif ($type == 'halfDonut') $type = 7;
		elseif ($type == 'table') $type = 9;
		elseif ($type < 1 || $type > 9) $type = 1;
		elseif (!is_numeric($type)) $type = 1;
		$this->width = $width;
		$this->height = $height;
		$this->max_label   = "";
		$this->max_value	= NULL;
		$this->num_labels = 0;
		$this->type = $type;
		$this->latin_notation	= false;
		$this->align = $this->dir == 'rtl' ? "end" : "start";
		$this->font = "Tahoma,Arial,sans-serif";
		if ($this->stream != 'svg')
		{
			$this->tsize = 12;
			$this->size = 9;
		}
		if ($this->dir == 'rtl')
		{
			$this->size += 1;
			$this->tsize += 1;
		}
	}

	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function setDirection($dir='ltr')
	{
		$this->dir = $dir;
	}
	
	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function setLanguage($lang='en')
	{
		$this->lang = $lang;
	}
	
	/**
	 * @param string|null $stream
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function setStream($stream = 'svg')
	{
		$this->stream = $stream;
		if ($this->stream == 'png')
		{
			$this->tsize = 12;
			$this->size = 9;
		}
	}

	/**
	 * @param array<array-key, mixed>|null $labels
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function setLabels($labels)
	{
		$this->labels = $labels;
	}

	/**
	 * @param string|null $label
	 * @param string|bool|null $type
	 * @param bool $fill
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function addSerie($label, $data, $type = false, $fill = false)
	{
		if ($label == false)
			$label = "Serie $this->num_series";
		$type = $type == 'line' ? 4 : ($type == 'spline' ? 8 : $type);
		$this->series[$this->num_series]['label'] = $label;
		$this->series[$this->num_series]['data'] = $data;
		$this->series[$this->num_series]['type'] = $type;
		$this->series[$this->num_series]['fill'] = $fill;
		$this->num_series++;
	}

	/**
	 * @param string|null $xtitle
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function setXTitle($xtitle)
	{
		$this->axis_x = $xtitle;
	}

	/**
	 * @param string|null $ytitle
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function setYTitle($ytitle)
	{
		$this->axis_y = $ytitle;
	}

	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function setTitle($title)
	{
		$this->title = $title;
	}

	/**
	 * @param string|null $dtitle
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function setDTitle($dtitle)
	{
		$this->donut_title = $dtitle; 
	}

	/**
	 * @param bool $values
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function setValues($values = false)
	{
		$this->set_values = $values;
	}

	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function setOutDir($dir)
	{
		$this->out_dir = $dir;
	}

	/** @return bool */
	function isEmpty()
	{
		return (!isset($this->series[0]['data']) || count($this->series[0]['data']) == 0);
	}

	/**
	 * @param string|null $save
	 * @return void
	 */
	function display($save="")
	{
		$this->encoding = strtoupper($_SESSION['language']->encoding);
		$this->title_height			= (!empty($this->title)) ? $this->tsize + 15 : 0;
		$this->bar_width			= ($this->type == 1) ? 20 : 15;
		$this->space_between_dots	= 40;
		$this->higher_value			= 0;
		$this->higher_value_str		= 0;
		$this->graphic_area_width	= 0;
		$this->graphic_area_height	= 0;
		$this->graphic_area_x1		= 30;
		$this->graphic_area_y1		= 30 + $this->title_height;
		$this->num_labels			= count($this->labels);
		$this->max_legend_str		= "";
		$this->legend_box_height  += ((float)(14 * $this->num_labels));
		for ($i = 0; $i < $this->num_labels; $i++)
		{
			if (strlen($this->labels[$i]) > MAXLEN)
				$this->labels[$i] = substr($this->labels[$i], 0, MAXLEN)."...";
			if (strlen($this->labels[$i]) > strlen($this->max_label))
				$this->max_label = $this->labels[$i];
		}
		for ($j = 0; $j < $this->num_series; $j++)
		{
			$this->sum_total[$j] = 0;
			if (strlen($this->series[$j]['label']) > strlen($this->max_legend_str))
				$this->max_legend_str = $this->series[$j]['label'];
			for ($i = 0; $i < $this->num_labels; $i++)
			{
				if ($this->series[$j]['data'][$i] > $this->max_value)
					$this->max_value = $this->series[$j]['data'][$i];
				$this->sum_total[$j] += $this->series[$j]['data'][$i];
			}
		}
		$this->max_value = number_format(round($this->max_value, 1), 1, ".", "");
		if ($this->num_series)
			$this->legend_exists = true;
		if ($this->type == 8 && $this->num_labels < 4)
			$this->type = 4;
		$this->calculate_higher_value();
		$this->calculate_width();
		$this->calculate_height();
		if ($this->type == 9)
		{
			$this->draw_table();
			return;
		}
		$this->create_graphic($save);
	}

	/**
	 * @param string|null $save
	 */
	function create_graphic($save="")
	{
		if ($this->stream == 'svg')
		{
			$this->svg = new SVG($this->width, $this->height, $this->dir, $this->lang);
			$this->svg->style("svg{font-family:{$this->font};background-color:inherit}.tips:hover{cursor:pointer;opacity:0.8;}");
			$this->load_color_palette($this->type);
		}
		else
		{
			$this->img = imagecreatetruecolor($this->width, $this->height);
			$this->load_color_palette($this->type);
			// Fill background
			imagefill($this->img, 0, 0, $this->color['background']);
		}
		// Draw title
		if (!empty($this->title))
		{
			if ($this->stream == 'svg')
			{
				$center = (float)$this->width / 2.0;
				$this->svg->text($center, 20, $this->title, $this->tsize, $this->color['title'], false, "middle");
			}
			else
			{
				$center = ((float)$this->width / 2.0) - ($this->string_width($this->title, $this->tsize) / 2);
				$this->_imagestring($this->img, $this->tsize, $center, 10, $this->title, $this->color['title']);
			}
		}
		// Draw axis and background lines for "vertical bars", "dots", "lines" and "splines"
		if (preg_match("/^(1|3|4|8)$/", $this->type))
		{
			if ($this->legend_exists == true)
			{
				$this->draw_legend();
			}
			$higher_value_y	   = (float)$this->graphic_area_y1 + ((float)(0.1 * (float)$this->graphic_area_height));
			$this->higher_value_size = 0.9 * (float)$this->graphic_area_height;
			$less  = 7 * strlen($this->higher_value_str);
			if ($this->stream == 'svg')
			{
				$this->svg->line($this->graphic_area_x1, $higher_value_y, $this->graphic_area_x2, $higher_value_y, $this->color['bg_lines']);
				$this->svg->text((float)$this->graphic_area_x1-(float)$less-7, $higher_value_y, $this->higher_value_str, $this->size, $this->color['axis_values'], false, $this->align);
			}
			else
			{
				imageline($this->img, $this->graphic_area_x1, $higher_value_y, $this->graphic_area_x2, $higher_value_y, $this->color['bg_lines']);
				$this->_imagestring($this->img, $this->size, ((float)$this->graphic_area_x1-(float)$less-7), ((float)$higher_value_y-7.0), $this->higher_value_str, $this->color['axis_values']);
			}
			for ($i = 1; $i < 10; $i++)
			{
				$dec_y = (float)$i * ((float)((float)$this->higher_value_size / 10.0));
				$x1 = round($this->graphic_area_x1);
				$y1 = round((float)$this->graphic_area_y2 - (float)$dec_y);
				$x2 = round($this->graphic_area_x2);
				$y2 = $y1;
				if ($this->stream == 'svg')
					$this->svg->line($x1, $y1, $x2, $y2, $this->color['bg_lines']);
				else
					imageline($this->img, $x1, $y1, $x2, $y2, $this->color['bg_lines']);
				if ($i % 2 == 0) 
				{
					$value = $this->number_formated((float)$this->higher_value * (float)$i / 10, $this->dec1);
					$len1 = strlen($this->higher_value_str);
					$len2 = strlen($value);
					if ($len2 < $len1)
						$len2 += ($len1-$len2-1);
					$less = 7 * $len2;
					if ($this->stream == 'svg')
						$this->svg->text((float)$x1-(float)$less-7, ($y1), $value, $this->size, $this->color['axis_values'], false, $this->align);
					else
						$this->_imagestring($this->img, $this->size, ((float)$x1-(float)$less-7), ((float)$y2-7.0), $value, $this->color['axis_values']);
				}
			}
			// Axis X
			if ($this->stream == 'svg')
			{
				$this->svg->text((float)$this->graphic_area_x2+10.0, (float)$this->graphic_area_y2+10.0, $this->axis_x, $this->size, $this->color['title'], false, $this->align);
				$this->svg->line($this->graphic_area_x1, $this->graphic_area_y2, $this->graphic_area_x2, $this->graphic_area_y2, $this->color['axis_line']);
			}
			else
			{
				$this->_imagestring($this->img, $this->size, (float)$this->graphic_area_x2+20.0, (float)$this->graphic_area_y2+3.0, $this->axis_x, $this->color['title']);
				imageline($this->img, $this->graphic_area_x1, $this->graphic_area_y2, $this->graphic_area_x2, $this->graphic_area_y2, $this->color['axis_line']);
			}
			// Axis Y
			if ($this->stream == 'svg')
			{
				$lex = $this->dir == 'rtl' ? 35 : 25; 
				$this->svg->text($lex, (float)$this->graphic_area_y1-10.0, $this->axis_y, $this->size, $this->color['title'], false, $this->align);
				$this->svg->line($this->graphic_area_x1, $this->graphic_area_y1, $this->graphic_area_x1, $this->graphic_area_y2, $this->color['axis_line']); 
			}
			else
			{
				$this->_imagestring($this->img, $this->size, 10, (float)$this->graphic_area_y1-20.0, $this->axis_y, $this->color['title']);
				imageline($this->img, $this->graphic_area_x1, $this->graphic_area_y1, $this->graphic_area_x1, $this->graphic_area_y2, $this->color['axis_line']);
			}
		}
		// Draw axis and background lines for "horizontal bars"
		else if ($this->type == 2)
		{
			if ($this->legend_exists == true)
			{
				$this->draw_legend();
			}
			$this->higher_value_size = 0.9 * (float)$this->graphic_area_width;
			if ($this->stream == 'svg')
			{
				$this->svg->line(((float)$this->graphic_area_x1+(float)$this->higher_value_size), $this->graphic_area_y1, ((float)$this->graphic_area_x1+(float)$this->higher_value_size), $this->graphic_area_y2, $this->color['bg_lines']);
				$this->svg->text((((float)$this->graphic_area_x1+(float)$this->higher_value_size) - ($this->string_width($this->higher_value, $this->size)/2)), ((float)$this->graphic_area_y2 + 20.0), $this->higher_value_str, $this->size, $this->color['axis_values'], false, $this->align);
			}
			else
			{
				imageline($this->img, ((float)$this->graphic_area_x1+(float)$this->higher_value_size), $this->graphic_area_y1, ((float)$this->graphic_area_x1+(float)$this->higher_value_size), $this->graphic_area_y2, $this->color['bg_lines']);
				$this->_imagestring($this->img, $this->size, (((float)$this->graphic_area_x1+(float)$this->higher_value_size) - ($this->string_width($this->higher_value, $this->size)/2)), ((float)$this->graphic_area_y2+2.0), $this->higher_value_str, $this->color['axis_values']);
			}
			for ($i = 1, $alt = 15; $i < 10; $i++)
			{
				$dec_x = number_format(round((float)$i * ((float)((float)$this->higher_value_size	 / 10.0)), 1), 1, ".", "");
				if ($this->stream == 'svg')
					$this->svg->line(($this->graphic_area_x1+$dec_x), $this->graphic_area_y1, ($this->graphic_area_x1+$dec_x), $this->color['bg_lines']);
				else
					imageline($this->img, ($this->graphic_area_x1+$dec_x), $this->graphic_area_y1, ($this->graphic_area_x1+$dec_x), $this->graphic_area_y2, $this->color['bg_lines']);
				if ($i % 2 == 0) 
				{
					$alt   = (strlen($this->max_value) > 4 && $alt != 15) ? 15 : 2;
					$value = $this->number_formated((float)$this->higher_value * (float)$i / 10, $this->dec1);
					if ($this->stream == 'svg')
						$this->svg->text((($this->graphic_area_x1+$dec_x) - ($this->string_width($this->higher_value, $this->size)/2)), ((float)$this->graphic_area_y2+(float)$alt+18), $value, $this->size, $this->color['axis_values'], false, $this->align);
					else
						$this->_imagestring($this->img, $this->size, (($this->graphic_area_x1+$dec_x) - ($this->string_width($this->higher_value, $this->size)/2)), ($this->graphic_area_y2), $value, $this->color['axis_values'], $alt);
				}
			}
			// Axis X
			if ($this->stream == 'svg')
			{
				$this->svg->text((float)$this->graphic_area_x2+10.0, ((float)$this->graphic_area_y2+10.0), $this->axis_y, $this->size, $this->color['title'], false, $this->align);
				$this->svg->line($this->graphic_area_x1, $this->graphic_area_y2, $this->graphic_area_x2, $this->graphic_area_y2, $this->color['axis_line']);
			}
			else
			{
				$this->_imagestring($this->img, $this->size, ((float)$this->graphic_area_x2+10.0), ((float)$this->graphic_area_y2+3.0), $this->axis_y, $this->color['title']);
				imageline($this->img, $this->graphic_area_x1, $this->graphic_area_y2, $this->graphic_area_x2, $this->graphic_area_y2, $this->color['axis_line']);
			}
			// Axis Y
			if ($this->stream == 'svg')
			{
				$lex = ($this->string_width($this->max_label, $this->size)) / 2;
				$this->svg->text($lex, ((float)$this->graphic_area_y1-10.0), $this->axis_x, $this->size, $this->color['title'], false, $this->align);
				$this->svg->line($this->graphic_area_x1, $this->graphic_area_y1, $this->graphic_area_x1, $this->graphic_area_y2, $this->color['axis_line']);
			}
			else
			{
				$this->_imagestring($this->img, $this->size, 20, ((float)$this->graphic_area_y1-20.0), $this->axis_x, $this->color['title']);
				imageline($this->img, $this->graphic_area_x1, $this->graphic_area_y1, $this->graphic_area_x1, $this->graphic_area_y2, $this->color['axis_line']);
			}
		}
		else if (preg_match("/^(5|6|7)$/", $this->type))
		{
			// Draw legend box for "pie", "donut" or "half donut"
			$this->draw_legend();
		}
		$average_width = (float)$this->graphic_area_width / (float)$this->num_labels;
		$biggest_width = $this->string_width($this->max_label, $this->size) + 10;
		if ($biggest_width > $average_width)
			$this->angle = -20;
		else
			$this->angle = 0;
		/**
		* Draw graphic: VERTICAL BARS
		*/
		if ($this->type == 1)
		{
			$this->draw_vertical_bars();
		}
		/**
		* Draw graphic: HORIZONTAL BARS
		*/
		else if ($this->type == 2)
		{
			$this->draw_horizontal_bars();
		}
		/**
		* Draw graphic: DOTS. LINE or SPLINE
		*/
		else if (preg_match("/^(3|4|8)$/", $this->type))
		{
			$this->draw_lines();
			if ($this->stream != 'svg' && $this->type == 8)
				$this->draw_splines();
		}
		/**
		* Draw graphic: PIE, DONUT or HALF DONUT
		*/
		else if (preg_match("/^(5|6|7)$/", $this->type))
		{
			// Draw PIE, DONUT OR HALF DONUT
			$this->draw_pie();
		}
		if ($this->stream == 'svg')
		{
			$this->svg->close();
			return $this->svg->draw();
			//$this->out_dir =	company_path(). '/pdf_files/';
			//$filename = $this->out_dir.uniqid().".svg";
			//$this->svg->save($filename);
			//echo "<img src='$filename' border='0' alt='$this->title'>";
		}
		else
		{
			$filename = $this->out_dir.uniqid().".".$this->stream;
			if ($this->stream == 'png')
				imagepng($this->img, $save != "" ? $save : $filename);
			elseif ($this->stream == 'jpg')
				imagejpeg($this->img, $save != "" ? $save : $filename);
			elseif ($this->stream == 'gif')	
				imagegif($this->img, $save != "" ? $save : $filename);
			echo "<img src='$filename' border='0' alt='$this->title'>";
			/*
			header('Content-type: image/png');
			imagepng($this->img, NULL, 5);
			*/
			imagedestroy($this->img);
		}
	}

	/** @return void */
	function draw_vertical_bars()
	{
		$x	 = (float)$this->graphic_area_x1 + 10.0;
		$oldx = $oldy = 0;
		$p = array();
		foreach ($this->labels as $i => $parameter)
		{
			for ($j = 0; $j < $this->num_series; $j++)
			{
				$size = round($this->series[$j]['data'][$i] * $this->higher_value_size / $this->higher_value);
				$x1	  = $x;
				$y1	  = ((float)((float)$this->graphic_area_y2 - (float)$size)) + 1.0;
				$x2	  = $this->bar_width;
				$y2	  = $this->graphic_area_y2;
				if ($this->series[$j]['type'] == 8 && $this->num_labels < 4)
					$this->series[$j]['type'] = 4;
				if ($this->stream == 'svg')
				{
					if ($this->series[$j]['type'] == 4 || $this->series[$j]['type'] == 8) // line
					{
						if (!isset($this->color['line'][0]))
							$this->load_color_palette($this->series[$j]['type']);
						if ($i == $this->num_labels - 1)
						{
							$dx = ((float)($this->num_series-1)) * ((float)((float)$this->bar_width + 10.0));
							$xx = (float)$this->graphic_area_x1 + (float)$dx;
							foreach($this->series[$j]['data'] as $i => $v)
							{
								$size  = round($this->series[$j]['data'][$i] * $this->higher_value_size / $this->higher_value);
								$y = (float)$this->graphic_area_y2 - (float)$size;
								$p[$i][0] = $xx;
								$p[$i][1] = $y;
								$xx += (float)$dx;
							}
							$path = "M ".$p[0][0].",".$p[0][1];
							foreach ($p as $ix => $y)
							{
								$this->svg->circle($p[$ix][0], $p[$ix][1], 2.5, $this->color['line'][$j], 2, $this->color['line'][$j], "class=\"tips\"", (string)$this->series[$j]['label'].": ".number_format($this->series[$j]['data'][$ix], 2)); 
								if ($ix != 0)
								{
									if ($this->series[$j]['type'] == 4) // normal line
										$path .= " L".$p[$ix][0].",".$p[$ix][1];
									elseif ($this->series[$j]['type'] == 8) // spline
										$path .= $this->_splineCommand($p[$ix], $ix, $p);
								}
							}
							if ($this->series[$j]['fill'] == true)
							{
								$fpath = $path;
								$fpath .= " L ".((float)$p[$ix][0]+1.0).",".((float)$this->graphic_area_y2-2.0)." L ".$p[0][0].",".((float)$this->graphic_area_y2-2.0)." L ".$p[0][0].",".$p[0][1];
								$this->svg->path($fpath, $this->color['line_light'][$j], 1, $this->color['line_light'][$j]);
							}
							$this->svg->path($path, $this->color['line'][$j], 3, "none");
						}
					}
					else
					{
						$this->svg->rect( $x1, $y1, $x2, (float)$y2-(float)$y1, $this->color['bars'][$j], 1, $this->color['bars'][$j], "class=\"tips\"", (string)$this->series[$j]['label'].": ".number_format($this->series[$j]['data'][$i], 2));
						$x += (float)((float)$this->bar_width + 5.0);
					}
				}
				else
				{
					if ($this->series[$j]['type'] == 4) // line
					{
						if (!isset($this->color['line'][0]))
							$this->load_color_palette($this->series[$j]['type']);
						if ($i > 0)
						{
							$this->_imageline($this->img, $oldx, $oldy, $x1, $y1, $this->color['line'][$j], 3);
							if ($this->series[$j]['fill'] == true)
							{
								$pt = array($oldx, (float)$oldy+3.0, $x1, (float)$y1 + 3.0, $x1, (float)$this->graphic_area_y2-2.0,	$oldx, (float)$this->graphic_area_y2-2.0, $oldx, (float)$oldy+4.0);
								imagefilledpolygon($this->img, $pt, 5, $this->color['line_light'][$j]);
							}
						}
						imagefilledrectangle($this->img, (float)$x1-2.0, (float)$y1-2.0, (float)$x1+2.0, (float)$y1+2.0, $this->color['line'][$j]);
						$oldx = $x1;
						$oldy = $y1;
						$x += 5.0;
					}
					elseif ($this->series[$j]['type'] == 8) // spline
					{
						if (!isset($this->color['line'][0]))
							$this->load_color_palette($this->series[$j]['type']);
						imagefilledrectangle($this->img, (float)$x1-2.0, (float)$y1-2.0, (float)$x1+2.0, (float)$y1+2.0, $this->color['line'][$j]);
						$p[$x1] = (float)$this->graphic_area_y2 - (float)$size;
						if ($i == $this->num_labels - 1)
						{
							$r = $this->imageSpline($p);
							if ($r == false)
								break;
							while (list ($xx, $yy) = each($r)) 
							{
								imagefilledellipse($this->img, round($xx), (float)round($yy)+1.0, 3, 3, $this->color['line'][$j]);
								if ($this->series[$j]['fill'] == true)
								{
									$this->_imageline($this->img, round($xx), round($yy+4), round($xx), (float)$this->graphic_area_y2-2.0, $this->color['line_light'][$j], 3);
								}
							}
						}
						$x += 5.0;
					}
					else
					{
						$xx2 = (float)$x2 + (float)$x1;
						imageline($this->img, ((float)$x1+1.0), ((float)$y1-1.0), $xx2, ((float)$y1-1.0), $this->color['bars'][$j]);
						imageline($this->img, ((float)$xx2+1.0), ((float)$y1-1.0), ((float)$xx2+1.0), $y2, $this->color['bars'][$j]);
						imageline($this->img, ((float)$xx2+2.0), ((float)$y1-1.0), ((float)$xx2+2.0), $y2, $this->color['bars'][$j]);
						imagefilledrectangle($this->img, $x1, $y1, $xx2, $y2, $this->color['bars'][$j]);
						$x += (float)((float)$this->bar_width + 5.0);
					}
				}
				if ($j == 0)
				{
					if ($this->stream == 'svg')
					{
						$lex = $this->angle == 0 ? (float)$x1 + 40.0 : (float)$x1+20.0;
						$al = $this->dir == 'rtl' ? "start" : "end";
						$this->svg->text($lex, ((float)$y2+15.0), $parameter, $this->size, $this->color['axis_values'], false, $al, $this->angle);
					}
					else
					{
						$this->_imagestring($this->img, $this->size, $x1, ((float)$y2+2.0), $parameter, $this->color['axis_values'], 0, $this->angle);
					}
				}
			}
			$x += 10.0;
		}
	}

	/** @return void */
	function draw_horizontal_bars()
	{
		$y = 10;
		$label_len = $this->string_width($this->max_label, $this->size);
		foreach ($this->labels as $i => $parameter)
		{
			for ($j = 0; $j < $this->num_series; $j++)
			{
				$size = round($this->series[$j]['data'][$i] / $this->higher_value * $this->higher_value_size);
				$x1	  = (float)$this->graphic_area_x1 + 1.0;
				$y1	  = (float)$this->graphic_area_y1 + (float)$y;
				$x2	  = (float)$x1 + (float)$size;
				$y2	  = (float)$y1 + (float)$this->bar_width;
				if ($this->stream == 'svg')
				{
					$this->svg->rect($x1, $y1, (float)$x2-(float)$x1, (float)$y2-(float)$y1, $this->color['bars'][$j], 1, $this->color['bars'][$j], "class=\"tips\"", (string)$this->series[$j]['label'].": ".number_format($this->series[$j]['data'][$i], 2));
					$this->svg->text(((float)$x2+10.0), ((float)$y1+12.0), $this->number_formated($this->series[$j]['data'][$i], $this->dec2), $this->size, $this->color['bars'][$j], false, $this->align);
					$y	 += (float)($this->bar_width + 1);
				}
				else
				{
					$this->_imageline($this->img, ($x1), ((float)$y2+1.0), $x2, ((float)$y2+1.0), $this->color['bars'][$j], 3);
					imagefilledrectangle($this->img, $x1, $y1, $x2, $y2, $this->color['bars'][$j]);
					$this->_imagestring($this->img, $this->size, ((float)$x2+7.0), ((float)$y1+2.0), $this->number_formated($this->series[$j]['data'][$i], $this->dec2), $this->color['bars'][$j]);
					$y	 += (float)($this->bar_width + 4);
				}
			}
			if ($this->stream == 'svg')
			{
				$lex = $this->dir == 'rtl' ? $label_len : 20;
				$this->svg->text($lex, ($y2-5), $parameter, $this->size, $this->color['axis_values'], false, "start");
				$y += 8.0;
			}
			else
			{
				$lex = $this->dir == 'rtl' ? $label_len+10 : 20;
				$this->_imagestring($this->img, $this->size, $lex, ($y1-5), $parameter, $this->color['axis_values'], 0, 0, $this->align);
				$y	 += 10.0;
			}
		}
	}

	/** @return void */
	function draw_lines()
	{
		for ($j = 0; $j < $this->num_series; $j++)
		{
			$x = (float)$this->graphic_area_x1+1.0;
			$oldx = $oldy = 0;
			if ($this->type == 8 && $this->num_labels < 4)
				$this->type = 4;
			if ($this->stream == 'svg')
			{
				$p = array();
				foreach ($this->labels as $i => $parameter)
				{
					$size  = round($this->series[$j]['data'][$i] * $this->higher_value_size / $this->higher_value);
					$y = (float)$this->graphic_area_y2 - (float)$size;
					$p[$i][0] = $x;
					$p[$i][1] = $y;
					if ($j == 0)
					{
						$lex = $this->angle == 0 ? (float)$x + 30.0 : (float)$x+10.0;
						$al = $this->dir == 'rtl' ? "start" : "end";
						$this->svg->text($lex, ((float)$this->graphic_area_y2+15.0), $parameter, $this->size, $this->color['axis_values'], false, $al, $this->angle);
					}
					$x += (float)$this->space_between_dots;
				}
				$path = "M ".$p[0][0].",".$p[0][1];
				foreach ($p as $i => $y)
				{
					$this->svg->circle($p[$i][0], $p[$i][1], 2.5, $this->color['line'][$j], 3, $this->color['line'][$j], "class=\"tips\"", (string)$this->series[$j]['label'].": ".number_format($this->series[$j]['data'][$i], 2)); 
					if ($i != 0)
					{
						if ($this->type == 4) // normal line
							$path .= " L".$p[$i][0].",".$p[$i][1];
						elseif ($this->type == 8) // spline
							$path .= $this->_splineCommand($p[$i], $i, $p);
					}
				}
				if ($this->series[$j]['fill'] == true)
				{
					$fpath = $path;
					$fpath .= " L ".((float)$p[$i][0]+1.0).",".((float)$this->graphic_area_y2-1.0)." L ".$p[0][0].",".((float)$this->graphic_area_y2-1.0)." L ".$p[0][0].",".$p[0][1];
					$this->svg->path($fpath, $this->color['line_light'][$j], 1, $this->color['line_light'][$j]);
				}
				$this->svg->path($path, $this->color['line'][$j], 3, "none");
				$x += (float)$this->space_between_dots;
			}
			else
			{
				foreach ($this->labels as $i => $parameter)
				{
					$size  = round($this->series[$j]['data'][$i] * $this->higher_value_size / $this->higher_value);
					$y = (float)$this->graphic_area_y2 - (float)$size;
					
					if ($this->type == 4 && $i > 0) // lINES
					{
						$this->_imageline($this->img, $oldx, $oldy, $x, $y, $this->color['line'][$j], 3);
						if ($this->series[$j]['fill'] == true)
						{
							$pt = array($oldx, (float)$oldy+3.0, $x, (float)$y + 3.0, $x, (float)$this->graphic_area_y2-2.0,	$oldx, (float)$this->graphic_area_y2-2.0, $oldx, (float)$oldy+4.0);
							imagefilledpolygon($this->img, $pt, 5, $this->color['line_light'][$j]);
						}
					}
					imagefilledrectangle($this->img, (float)$x-2.0, (float)$y-2.0, (float)$x+2.0, (float)$y+2.0, $this->color['line'][$j]);
					$oldx = $x;
					$oldy = $y;
					if ($j == 0)
						$this->_imagestring($this->img, $this->size, $oldx, ((float)$this->graphic_area_y2+2.0), $parameter, $this->color['axis_values'], 0, $this->angle);
					$x += (float)$this->space_between_dots;
				}
			}
		}
	}

	/** @return void */
	function draw_splines()
	{
		for ($j = 0; $j < $this->num_series; $j++)
		{
			$n = (float)$this->graphic_area_x1+1.0;
			foreach ($this->labels as $i => $parameter)
			{
				$size  = round($this->series[$j]['data'][$i] * $this->higher_value_size / $this->higher_value);
				$y = (float)$this->graphic_area_y2 - (float)$size;
				$p[$n] = $y;
				$n += (float)$this->space_between_dots;
			}
			$r = $this->imageSpline($p);
			reset($r);
			$oldx = key($r);
			$oldy = current($r);
			foreach ($r as $x => $y) 
			{
				imagefilledellipse($this->img, round($x), (float)round($y)+1.0, 3, 3, $this->color['line'][$j]);

				if ($this->series[$j]['fill'] == true)
				{
					$this->_imageline($this->img, round($x), round($y+4), round($x), (float)$this->graphic_area_y2-2.0, $this->color['line_light'][$j], 1);
				}
			}
		}
	}

	/**
	 * @param int|null $step
	 * @param int|null $minx
	 * @param int|null $maxx
	 * @return array<array-key, mixed>|false
	 */
	function imageSpline($p, $step = 1, $minx = -1, $maxx = -1)
	{
		$splines = array();
		if (count($p) < 4)
			return false;
		$cx = $cy = $p1 = array();
		ksort($p);
		foreach ($p as $x => $y)
		{
			$cx[] = $x;
			$cy[] = $y;
		}
		if ($minx == -1)
			$minx = min($cx);
		if ($maxx == -1)
			$maxx = max($cx);
		$n = count($cx);
		for ($i = 0; $i < $n; ++$i) 
		{
			$splines[$i]['x'] = $cx[$i];
			$splines[$i]['a'] = $cy[$i];
		}
		$splines[0]['c'] = $splines[$n - 1]['c'] = 0;
		$alpha[0] = $beta[0] = 0;
		for ($i = 1; $i < $n - 1; ++$i) 
		{
			$h_i = $cx[$i] - $cx[$i - 1];
			$h_i1 = $cx[$i + 1] - $cx[$i];
			$A = $h_i;
			$C = 2.0 * ($h_i + $h_i1);
			$B = $h_i1;
			$F = 6.0 * (($cy[$i + 1] - $cy[$i]) / $h_i1 - ($cy[$i] - $cy[$i - 1]) / $h_i);
			$z = ($A * $alpha[$i - 1] + $C);
			$alpha[$i] = - $B / $z;
			$beta[$i] = ($F - $A * $beta[$i - 1]) / $z;
		}
		for ($i = $n - 2; $i > 0; --$i) 
			$splines[$i]['c'] = (float)$alpha[$i] * (float)$splines[$i + 1]['c'] + $beta[$i];
		for ($i = $n - 1; $i > 0; --$i) 
		{
			$h_i = $cx[$i] - $cx[$i - 1];
			$splines[$i]['d'] = ((float)$splines[$i]['c'] - (float)$splines[$i - 1]['c']) / $h_i;
			$splines[$i]['b'] = $h_i * (2.0 * (float)$splines[$i]['c'] + (float)$splines[$i - 1]['c']) / 6.0 + ($cy[$i] - $cy[$i - 1]) / $h_i;
		}
		for ($x = $minx; $x <= $maxx; $x += $step) 
		{
			$n = count($splines);
			if ($x <= $splines[0]['x'])	 {
				$s = $splines[1];
			} else {
				if ($x >= $splines[$n - 1]['x']) {
					$s = $splines[$n - 1];
				} else {
					$i = 0;
					$j = $n - 1;
					while ((float)$i + 1.0 < $j) {
						$k = (float)$i + (float)(((float)((float)$j - (float)$i)) / 2.0);
						if ($x <= $splines[round($k)]['x']) {
							$j = $k;
						} else {
							$i = $k;
						}
					}
					$s = $splines[round($j)];
				}
			}
			$dx = ($x - $s['x']);
			$p1[$x] = (float)$s['a'] + ((float)$s['b'] + ((float)$s['c'] / 2.0 + (float)$s['d'] * $dx / 6.0) * $dx) * $dx;
		}
		return $p1;
	}

	/** @return void */
	function draw_pie()
	{
		$n = count(self::$palette);
		if ($this->stream == 'svg')
		{
			$cx = ((float)((float)$this->graphic_area_x1 + (float)$this->graphic_area_x2)) / 2.0;
			$cy = ((float)((float)$this->graphic_area_y1 + (float)$this->graphic_area_y2)) / 2.0;
			$width	  = $this->graphic_area_width;
			$height	  = $this->graphic_area_height;
			if ($this->sum_total[0] == 0)
				$this->sum_total[0] = 1;
			$r = (float)$cx * 0.7;
			$rc = $this->type == 5 ? 0.01 : (float)$r - 25.0;
			$d = $this->type == 7 ? 180 : 360; // half donut or all
			$area = (float)$d / (float)$this->sum_total[0]; 
			$start = $this->type == 7 ? 180 : 90;
			$rad = pi() / 180.;
			$out='';
			$gap=0.01;
			$ang1=0;	  // start angle
			foreach ($this->labels as $i => $num)
			{
				$n = count(self::$palette);
				$pct[$i] = round($this->series[0]['data'][$i] / $this->sum_total[0] * 100, 1);
				$dang = $this->series[0]['data'][$i] * $area;		// delta angle
				$laf  = $dang > 180 ? 1 : 0; // Large Arc Flag
				$ang2 = $ang1 + $dang;		// second angle
				$a = ($ang1 - $start) * $rad + asin($gap / $rc); 
				$p1 = sprintf('%0.2f,%0.2f', (float)$cx + (float)($rc * cos($a)), (float)$cy + (float)($rc * sin($a)));
				$a = ($ang1 - $start) * $rad + asin($gap / $r); 
				$p2 = sprintf('%0.2f,%0.2f', (float)$cx + (float)($r * cos($a)), (float)$cy + (float)($r * sin($a)));
				$a = ($ang2 - $start) * $rad - asin($gap / $r); 
				$p3 = sprintf('%0.2f,%0.2f', (float)$cx + (float)($r * cos($a)), (float)$cy + (float)($r * sin($a)));
				$a = ($ang2 - $start) * $rad - asin($gap / $rc); 
				$p4 = sprintf('%0.2f,%0.2f', (float)$cx + (float)($rc * cos($a)), (float)$cy + (float)($rc * sin($a)));
				$a = ($ang1 - $start) * $rad + asin($gap / $rc); 
				$p5 = sprintf('%0.2f,%0.2f', (float)$cx + (float)($rc * cos($a)), (float)$cy + (float)($rc * sin($a)));
				$this->svg->path("M$p1 L$p2 A$r,$r 0 $laf,1 $p3 L$p4 A$rc,$rc 0 $laf,0 $p5", $this->color[$i % $n], 0, $this->color[$i % $n], "class=\"tips\"", "$num: ".number_format($pct[$i], 1)."&#x25;");
				$ang1=$ang2;
			}
			if (!empty($this->donut_title) && $this->type != 5)
			{
				// Display center text
				if ($this->type == 6)
					$cy += 5.0;
				$this->svg->text($cx, $cy, $this->donut_title, $this->size, $this->color[0], "style=\"font-size:18px;font-weight:bold;\"", "middle");
			}
		}
		else
		{
			$start	  = 0;
			$sizes	  = array();
			$degrees = $this->type == 7 ? 180 : 360; // half donut or all
			foreach ($this->labels as $i => $parameter)
			{
				if ($this->sum_total[0] == 0)
					$this->sum_total[0] = 1;
				if ($this->series[0]['data'][$i] < 0)
					$this->series[0]['data'][$i] = 0;
				$size	 = $this->series[0]['data'][$i] * $degrees / $this->sum_total[0];
				$size = round($size, 0);
				$sizes[] = $size;
				$start	+= (float)$size;
			}
			$center_x = ((float)((float)$this->graphic_area_x1 + (float)$this->graphic_area_x2)) / 2.0;
			$center_y = ((float)((float)$this->graphic_area_y1 + (float)$this->graphic_area_y2)) / 2.0;
			$width	  = $this->graphic_area_width;
			$height	  = $this->graphic_area_height;
			$start = $this->type == 7 ? 180 : 270;
			// Draw pieces
			foreach ($sizes as $i => $size)
			{
				$j = $i % $n;
				$color = 'arc_' . $j;
				if ($size >= 1)
					imagefilledarc($this->img, $center_x, $center_y, $width, $height, $start, ((float)$start+(float)$size), $this->color[$color], IMG_ARC_PIE);
				$start += (float)$size;
			}
			if ($this->type != 5) // DONUTS
			{
				// 85 pixels width hole
				$start = $this->type == 7 ? 180 : 0; 
				imagefilledarc($this->img, $center_x, $center_y, 85, 85, $start, 360, $this->color['background'], IMG_ARC_PIE);
				imagearc($this->img, $center_x, $center_y, 85, 85, $start, 360, $this->color['bg_legend']);
				imagearc($this->img, $center_x, $center_y, ((float)$width+1.0), ((float)$height+1.0), $start, 360, $this->color['bg_legend']);
				if (!empty($this->donut_title))
				{
					$plen = $this->string_width($this->donut_title, $this->size);
					$x1 = $center_x - $plen / 2 - 6;
					$co = $this->type == 7 ? 20 : 10; // HALF or ALL
					$this->_imagestring($this->img, $this->size + 4, $x1, (float)$center_y - (float)$co, $this->donut_title, $this->color['arc_0'], 2, 0);	
					$this->_imagestring($this->img, $this->size + 4, $x1+1, (float)$center_y - (float)$co, $this->donut_title, $this->color['arc_0'], 2, 0);	
				}
			}
		}
	}

	/** @return void */
	function draw_table()
	{
		$cs = $this->num_labels + 1;
		$html = "<table cellspacing='0' //style='width:{$this->width}px;font-family:{$this->font};font-size:12px;background:inherit;'>\n";
		if (!empty($this->title))
			$html .= "<tr><th colspan=$cs style='text-align:center;'><h3>$this->title</th><tr>\n";
		$html .= "<tr><th style='border-bottom:1px solid #444'>#</th>"; 
		for ($i = 0; $i < $this->num_labels; $i++)
			$html .= "<th style='text-align:right;border-bottom:1px solid #444'>{$this->labels[$i]}</th>";
		$html .= "</tr>\n";
		for ($j = 0; $j < $this->num_series; $j++)
		{
			$html .= "<tr><td style=''>{$this->series[$j]['label']}</td>";
			for ($i = 0; $i < $this->num_labels; $i++)
				$html .= "<td style='text-align:right;'>".number_format($this->series[$j]['data'][$i], 0)."</td>";
			$html .= "</tr>\n";
		}
		$html .= "</table>\n";
		echo $html;
	}

	/** @return void */
	function calculate_width()
	{
		$this->legend_box_width = ($this->legend_exists == true) ? ($this->string_width($this->max_legend_str, $this->size) + 70) : 0;
		switch ($this->type)
		{
			// Vertical bars or Table
			case 1:
			case 9:
				$this->graphic_area_x1	 += $this->string_width($this->higher_value_str, $this->size);
				if (!empty($this->width)) 
					$this->graphic_area_width = $this->width - $this->graphic_area_x1 - 30 - $this->legend_box_width;
				else
				{
					$gap_series = 10;
					$gap_labels = 5;
					$serie_width = 0;
					for ($j = 0; $j < $this->num_series; $j++)
					{
						if ($this->series[$j]['type'] != 4 && $this->series[$j]['type'] != 8)
							$serie_width += (float)$this->bar_width;
						$serie_width += (float)$gap_labels;
					}
					$serie_width += (float)$gap_series;
					$this->graphic_area_width = (float)$serie_width * (float)$this->num_labels;
					$this->width += (float)$this->graphic_area_width;
					$this->width += $this->graphic_area_x1;
					$this->width += $this->legend_box_width;
				}
				break;
			// Horizontal bars
			case 2:
				$this->graphic_area_x1 += $this->string_width($this->max_label, $this->size);
				if (!empty($this->width))
					$this->graphic_area_width = $this->width - $this->graphic_area_x1 - 30 - $this->legend_box_width;
				else
				{
					$this->graphic_area_width = ($this->string_width($this->higher_value_str, $this->size) > 50) ? (5 * ($this->string_width($this->higher_value_str, $this->size)) * 0.85) : 200;
					$this->width += $this->graphic_area_x1;
					$this->width += $this->graphic_area_width;
					$this->width += $this->legend_box_width;
				}
				break;
			// Dots
			case 3:
			// Lines
			case 4:
			// Splines
			case 8:
				$this->graphic_area_x1	 += $this->string_width(($this->higher_value_str), $this->size);
				if (!empty($this->width))
					$this->graphic_area_width = $this->width - $this->graphic_area_x1 - 30 - $this->legend_box_width;
				else
				{
					$this->graphic_area_width = (float)$this->space_between_dots * ((float)($this->num_labels - 1)) + 5;
					$this->width += (float)$this->graphic_area_width;
					$this->width += $this->graphic_area_x1 + 30;
					$this->width += $this->legend_box_width;
				}
				break;
			// Pie
			case 5:
			// Donut
			case 6:
			// Half Donut
			case 7:
				$this->legend_box_width	  += 60;
				if (!empty($this->width))
					$this->graphic_area_width = (float)$this->width - (float)$this->graphic_area_x1 - 30 - $this->legend_box_width;
				else
				{
					$this->graphic_area_width = 150;
					$this->width += (float)($this->graphic_area_x1 +20);
					$this->width += $this->legend_box_width;
					$this->width += $this->graphic_area_width;
					$this->width += 90;
				}
				break;
		}
		$this->graphic_area_width = max($this->graphic_area_width, $this->string_width($this->title, $this->size));
		$this->graphic_area_x2 = $this->graphic_area_x1 + $this->graphic_area_width;
		$this->legend_box_x1   = $this->graphic_area_x2 + 20;
		$this->legend_box_x2   = $this->legend_box_x1 + $this->legend_box_width;
	}

	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function calculate_height()
	{
		switch ($this->type)
		{
			// Vertical bars
			case 1:
				if (!empty($this->height))
					$this->graphic_area_height = (float)$this->height - (float)$this->title_height;
				else
				{
					$this->graphic_area_height = 150;
					$this->height += 65;
				}
				break;
		   // Horizontal bars
			case 2:
				if (!empty($this->height))
					$this->graphic_area_height = (float)$this->height - (float)$this->title_height;
				else
				{
					$gap_labels = 12;
					if ($this->stream != 'svg')
						$gap_labels += 6;
					$this->graphic_area_height = (float)$this->num_series * (float)$this->bar_width * $this->num_labels + $this->num_labels * $gap_labels;
					$this->height += 65;
				}
				break;
			// Dots
			case 3:
			// Lines
			case 4:
			// Spines
			case 8:
				if (!empty($this->height))
					$this->graphic_area_height = (float)$this->height - (float)$this->title_height;
				else
				{
					$this->graphic_area_height = 150;
					$this->height += 65;
				}
				break;
			// Pie
			case 5:
			// Donut
			case 6:
			// Half Donut
			case 7:
				if (!empty($this->height))
					$this->graphic_area_height = (float)$this->height - (float)$this->title_height;
				else
				{
					$this->graphic_area_height = 150;
					$this->height += 65;
				}
				break;
		}
		//if ($this->type == 7) // Half Donut;
		$this->graphic_area_y1 -= 10.0;
		$this->legend_box_y1   = $this->graphic_area_y1;
		$this->height += (float)($this->graphic_area_height + 40);
		$this->graphic_area_y2 = (float)$this->graphic_area_y1 + (float)$this->graphic_area_height;
		$this->legend_box_y2   = (float)$this->legend_box_y1 + (float)$this->legend_box_height;
	}

	/** @return void */
	function draw_legend()
	{
		$x1 = $this->legend_box_x1;
		$y1 = $this->legend_box_y1;
		$x2 = $this->legend_box_x2;
		$y2 = $this->legend_box_y2;
		if ($this->stream == 'svg')
			$this->svg->rect($x1, $y1, $x2, $y2, $this->color['bg_legend']);
		else
			imagefilledrectangle($this->img, $x1, $y1, $x2, $y2, $this->color['bg_legend']);
		$x = (float)$x1 + 10.0;
		$y = $y1;
		// Draw legend values for VERTICAL BARS, HORIZONTAL BARS, DOTS, LINES AND SPLINES
		if (preg_match("/^(1|2|3|4|8)$/", $this->type))
		{
			for ($j = 0; $j < $this->num_series; $j++)
			{
				$color = (preg_match("/^(1|2)$/", $this->type)) ? $this->color['bars'][$j]	 : $this->color['line'][$j];
				$len = $this->string_width($this->max_legend_str, $this->size)-20;
				if ($this->stream == 'svg')
				{
					$lex = $this->dir == 'rtl' ? $x1+$len : (float)$x1-5.0;
					$this->svg->circle($lex, (float)$y-5.0, 6, "#fff", 1, $color);
					//$this->svg->rect($lex, $y-10, 10, 10, "#fff", 1, $color);
					$lex = $this->dir == 'rtl' ? $x1+$len-15 : (float)$x-5.0; 
					$this->svg->text($lex, ((float)$y-2.0), $this->series[$j]['label'], $this->size, $this->color['axis_values'], false, "start");
				}
				else
				{
					$lex = $this->dir == 'rtl' ? $x1+$len+20 : (float)$x-10.0;
					imagefilledrectangle($this->img, $lex, $y, ($lex+10), ((float)$y+10.0), $color);
					imagerectangle($this->img, $lex, $y, ($lex+10), ((float)$y+10.0), $this->color['title']);
					$lex = $this->dir == 'rtl' ? $x1+$len+20 : (float)$x+10.0;
					$this->_imagestring($this->img, $this->size, $lex, ((float)$y-2.0), $this->series[$j]['label'], $this->color['axis_values'], 0, 0, $this->align);
				}
				$y += 20.0;
			}
		}
		// Draw legend values for PIE, DONUT or HALF DONUT
		else if (preg_match("/^(5|6|7)$/", $this->type))
		{
			$n = count(self::$palette);
			if ($this->sum_total[0] == 0)
				return;
			$label_len = $this->string_width($this->max_label, $this->size);
			//$value_len = $this->string_width($this->max_value, $this->size);
			$value_len = $this->string_width(number_format($this->max_value), $this->size);
			$total_len = $label_len + $value_len;
			$total_len = max($total_len, (float)$x2-(float)$x1);
			if (!empty($this->axis_x)) // Only first serie [0]
			{
				if ($this->stream == 'svg')
					$this->svg->text(((((float)$x1+(float)$x1+$total_len)/2) - (strlen($this->axis_x)*7/2)), $y, $this->axis_x." (".(string)$this->series[0]['label'].")", $this->size, $this->color['title'], false, $this->align);
				else
					$this->_imagestring($this->img, $this->size, (((float)$x1+(float)$x1+$total_len)/2 - strlen($this->axis_x)*7/2)+30, $y, 
					$this->axis_x."-".(string)$this->series[0]['label'], $this->color['title']);
				$y += 25.0;
			}
			foreach ($this->labels as $i => $parameter)
			{
				if ($this->set_values)
					$text = number_format($this->series[0]['data'][$i]);
				else
					$text = number_format($this->series[0]['data'][$i] * 100 / $this->sum_total[0], 2) . ' %';
				if ($this->stream == 'svg')
				{
					$j = $i % $n;
					$lex = $this->dir == 'rtl' ? $x1+$total_len-25 : $x1;
					$this->svg->circle($lex, (float)$y-5.0, 6, "#fff", 1, $this->color[$j]);
					// $this->svg->rect($lex-5, $y-10, 10, 10, "#fff", 1, $this->color[$j]);
					// Display label text
					$lex = $this->dir == 'rtl' ? $x1+$total_len-40 : (float)$x1+15.0;
					$this->svg->text($lex, ((float)$y-2.0), $parameter, $this->size, $this->color['axis_values'], false, "start");	
					$lex = $this->dir == 'rtl' ? $x1+$value_len-25 : $x1+$total_len;
					$al = $this->dir == 'rtl' ? "start" : "end";
					$this->svg->text($lex, ((float)$y-2.0), $text, $this->size, $this->color['axis_values'], false, $al);
					$lex2 = $this->dir == 'rtl' ? $x1+$total_len-25 : $x1+$total_len;
					$this->svg->line($x1, (float)$y+1.0, $lex2, (float)$y+1.0, $this->color['bg_lines']);
				}
				else
				{
					$j = $i % $n;
					$color = 'arc_' . $j;
					$width = $this->string_width($text, $this->size);
					imageline($this->img, (float)$x1+15.0, ((float)$y+11.0), (float)$x2+20.0, ((float)$y+11.0), $this->color['bg_lines']);
					$lex = $this->dir == 'rtl' ? (float)$x2+20.0 : $x;
					imagefilledrectangle($this->img, $lex, $y, ((float)$lex+10.0), ((float)$y+10.0), $this->color[$color]);
					imagerectangle($this->img, $lex, $y, ((float)$lex+10.0), ((float)$y+10.0), $this->color['title']);
					$lex = $this->dir == 'rtl' ? (float)$x2+5.0 : (float)$x+15.0;
					$this->_imagestring($this->img, $this->size, $lex, ((float)$y-2.0), $parameter, $this->color['axis_values'], 0, 0, $this->align);
					//$lex = $this->dir == 'rtl' ? $x+10 : $x2-$width;
					$lex = $this->dir == 'rtl' ? $x1+$value_len : $x2-$width;
					$al = $this->dir == 'rtl' ? "end" : $this->align;
					$this->_imagestring($this->img, $this->size, $lex, ((float)$y-2.0), $text, $this->color['axis_values'], 0, 0, $al);
				}
				$y += 18.0;
			}
		}
	}

	function string_width($string, $size) 
	{
		if ($string == NULL)
			return $size;
		$p = imageftbbox($size, 0, $this->fontfile, $string);
		return $p[4] - $p[0];
	}

	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function calculate_higher_value() 
	{
		$digits	  = strlen(round($this->max_value));
		$interval = pow(10, ($digits-1));
		$this->higher_value = round(((float)$this->max_value - ((float)(round($this->max_value) % $interval)) + $interval), 1);
		$this->higher_value_str = $this->number_formated($this->higher_value, $this->dec1);
	}

	/**
	 * @param int|null $dec_size
	 * @psalm-mutation-free
	 * @return string
	 */
	function number_formated($number, $dec_size = 1)
	{
		if ($this->latin_notation == true) 
			return number_format(round($number, $dec_size), $dec_size, ",", ".");
		return number_format(round($number, $dec_size), $dec_size, ".", ",");
	}

	/** @return void */
	function load_color_palette($type)
	{
		// The usual color palette. Change if you like. Must be 6 colors.
		$color = self::$palette;
		if ($this->stream == 'svg')
		{
			$this->color['title']		= sprintf("#%02x%02x%02x",	40,	 70, 130);
			$this->color['background']	= sprintf("#%02x%02x%02x", 255, 255, 255);
			$this->color['axis_values'] = sprintf("#%02x%02x%02x",	50,	 50,  50);
			$this->color['axis_line']	= sprintf("#%02x%02x%02x", 100, 100, 100);
			$this->color['bg_lines']	= sprintf("#%02x%02x%02x", 220, 220, 220);
			$this->color['bg_legend']	= sprintf("#%02x%02x%02x", 255, 255, 255);
			$this->color['shadow']		= sprintf("#%02x%02x%02x", 255, 255, 255);

			if (preg_match("/^(1|2)$/", $type)) // Vertical Bar or Horizontal bar
			{
				for ($j = 0; $j < $this->num_series; $j++)
				{
					$this->color['bars'][$j] = $color[$j];
				}
			}
			elseif (preg_match("/^(3|4|8)$/", $type)) // Dots, Lines or Splines
			{
				for ($j = 0; $j < $this->num_series; $j++)
				{
					$this->color['line'][$j] = $color[$j];
					$this->color['line_light'][$j] = $this->colorLuminate($color[$j], 0.8);
				}
			}
			elseif (preg_match("/^(5|6|7)$/", $type)) // Pie, Donut or Half Donut
			{
				$n = count($color);
				for ($j = 0; $j < $n; $j++) 
					$this->color[$j] = $color[$j];
			}
		}
		else
		{
			$this->color['title']		= imagecolorallocate($this->img,  40,  70,	130);
			$this->color['background']	= imagecolorallocate($this->img, 255, 255, 255);
			$this->color['axis_values'] = imagecolorallocate($this->img,  50,  50,	50);
			$this->color['axis_line']	= imagecolorallocate($this->img, 100, 100, 100);
			$this->color['bg_lines']	= imagecolorallocate($this->img, 220, 220, 220);
			$this->color['bg_legend']	= imagecolorallocate($this->img, 255, 255, 255);
			$this->color['shadow']		= imagecolorallocate($this->img, 255, 255, 255);

			if (preg_match("/^(1|2)$/", $type)) // Vertical Bar or Horizontal bar
			{
				for ($j = 0; $j < $this->num_series; $j++)
				{
					$c = $this->hex2rgb($color[$j]);
					$this->color['bars'][$j] = imagecolorallocate($this->img, $c[0], $c[1], $c[2]);
				}
			}
			else if (preg_match("/^(3|4|8)$/", $type)) // Dots, Lines or Splines
			{
				for ($j = 0; $j < $this->num_series; $j++)
				{
					$c = $this->hex2rgb($color[$j]);
					$this->color['line'][$j] = imagecolorallocate($this->img, $c[0], $c[1], $c[2]);
					$c = $this->hex2rgb($this->colorLuminate($color[$j], 0.8)); // lighten for Areas
					$this->color['line_light'][$j] = imagecolorallocate($this->img, $c[0], $c[1], $c[2]);
				}
			}
			else if (preg_match("/^(5|6|7)$/", $type)) // Pie, Donut or Half Donut
			{
				$n = count($color); 
				for ($j = 0; $j < $n; $j++) 
				{
					$c = $this->hex2rgb($color[$j]);
					$this->color["arc_{$j}"] = imagecolorallocate($this->img, $c[0], $c[1], $c[2]);
				}
			}
		}
	}

	/**
	 * @param string|null $hex
	 * @param float|null $percent
	 * @return non-falsy-string
	 */
	function colorLuminate($hex, $percent) // Color lightner, minus is darker
	{
		$hex = ltrim($hex, '#');
		if (strlen($hex) == 3) {
			$hex = $hex[0] . $hex[0] . $hex[1] . $hex[1] . $hex[2] . $hex[2];
		}
		$hex = array_map('hexdec', str_split($hex, 2));
		foreach ($hex as &$color) {
			$limit = $percent < 0 ? $color : 255 - $color;
			$value = ceil((float)$limit * (float)$percent);
			$color = str_pad(dechex((float)$color + (float)$value), 2, '0', STR_PAD_LEFT);
		}
		return '#' . implode($hex);
	}
	
	/**
	 * @param bool|GdImage|null $image
	 * @param int|null $thick
	 * @return bool
	 */
	function _imageline($image, $x1, $y1, $x2, $y2, $color, $thick = 1)
	{
		if ($thick == 1) 
			return imageline($image, $x1, $y1, $x2, $y2, $color);
		$t = (float)$thick / 2.0 - 0.5;
		if ($x1 == $x2 || $y1 == $y2)
			return imagefilledrectangle($image, round(min($x1, $x2) - $t), round(min($y1, $y2) - $t), round(max($x1, $x2) + $t), round(max($y1, $y2) + $t), $color);
		$k = ($y2 - $y1) / ($x2 - $x1); //y = kx + q
		$a = $t / sqrt(1.0 + (float)pow($k, 2));
		$points = array(
			round($x1 - (1+$k)*$a), round($y1 + (1-$k)*$a),
			round($x1 - (1-$k)*$a), round($y1 - (1+$k)*$a),
			round($x2 + (1+$k)*$a), round($y2 - (1-$k)*$a),
			round($x2 + (1-$k)*$a), round($y2 + (1+$k)*$a),
		);
		if (PHP_MAJOR_VERSION >= 8) { // num_points is deprecated
			imagefilledpolygon($image, $points, $color);
			return imagepolygon($image, $points, $color);
		}
		else {
			imagefilledpolygon($image, $points, 4, $color);
			return imagepolygon($image, $points, 4, $color);
		}
	}

	/**
	 * @param bool|GdImage|null $img
	 * @param int|null $alt
	 * @param int|null $angle
	 * @param string|bool|null $align
	 * @return void
	 */
	function _imagestring($img, $size, $x, $y, $string, $col, $alt=0, $angle=0, $align=false)
	{
		if ($align == 'end')
		{
			$p = imagettfbbox($size, $angle, $this->fontfile, $string); 
			$x -= $p[2];
		}
			
		if ($this->encoding != 'UTF-8') 
		{
			if (function_exists('iconv'))
				$string = iconv($this->encoding, 'UTF-8', $string);
			else
				$string = mb_convert_encoding($string, 'UTF-8', $this->encoding);
		}
		
		// Handling ev. RTL languages
		if ($alt)
		{
			if ($this->encoding == 'UTF-8' && $this->dir == 'rtl')
				$alt_len = 18;
			else
				$alt_len = 12;
			if (strlen($string) > $alt_len)
				$string = substr($string, 0, $alt_len);
		}		
		if ($this->encoding == 'UTF-8')
		{
			if ((bool)is_arabic($string))
				$string = arabic($string);
			elseif ((bool)is_hebrew($string))
				$string = hebrew($string);
		}
		$y += $size + 3;	
		if ($this->encoding == 'UTF-8' && (bool)is_arabic($string))
		{
			$size += 2;
			$string = str_replace(" ", "  ", $string);
		}	
		imagettftext($img, $size, $angle, round($x), $y  + $alt, $col, $this->fontfile, $string);
	}

	/**
	 * @param string|null $hex
	 * @psalm-pure
	 * @return list{float|int|null|string, float|int|null|string, float|int|null|string}
	 */
	function hex2rgb($hex)
	{
		list($r, $g, $b) = sscanf($hex, "#%02x%02x%02x");
		return array($r, $g, $b);
	}

	/**
	 * @param bool $reverse
	 * @return list{mixed, mixed}
	 */
	function _controlPoint($curr, $prev, $next, $reverse = false)
	{
		$p = $prev ? $prev : $curr;
		$n = $next ? $next : $curr;
		$smooth = 0.2;
		$lenx = $n[0] - $p[0];
		$leny = $n[1] - $p[1];
		$len = sqrt((float)pow($lenx, 2) + (float)pow($leny, 2));
		$angle = atan2($leny, $lenx);
		$len *= $smooth;
		$angle += ((float)($reverse ? pi() : 0));
		$x = $curr[0] + cos($angle) * $len;
		$y = $curr[1] + sin($angle) * $len;
		return array($x, $y);
	}

	/**
	 * @param array<array-key, mixed>|null $a
	 * @return non-falsy-string
	 */
	function _splineCommand($p, $i, $a)
	{
		$c = isset($a[$i - 1]) ? $a[$i - 1] : false;
		$d = isset($a[$i - 2]) ? $a[$i - 2] : false;
		$e = isset($a[$i + 1]) ? $a[$i + 1] : false;
		$cps = $this->_controlPoint($c, $d, $p);
		$cpe = $this->_controlPoint($p, $c, $e, true);
		return " C".round($cps[0],1).",".round($cps[1],1)." ".round($cpe[0],1).",".round($cpe[1], 1)." $p[0],$p[1]";
	}

	/** @return void */
	function clean_out_dir()
	{
		$dir = $this->out_dir;
		if ($d = @opendir($dir)) {
			while (($file = readdir($d)) !== false) {
				if (!is_file($dir.'/'.$file) || $file == 'index.php') continue;
				// then check to see if this one is too old
				$ftime = filemtime($dir.'/'.$file);
				// seems 3 min is enough for any report download, isn't it?
				if (time()-$ftime > 180){
					unlink($dir.'/'.$file);
				}
			}
			closedir($d);
		}
	}
}

class SVG
{
	/**
	 * @var string
	 */
	var $svg;

	/** @psalm-mutation-free */
	function __construct($width=0, $height=0, $dir='ltr', $lang='en')
	{
		$this->svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" direction=\"$dir\" xml:lang=\"$lang\" width=\"$width\" height=\"$height\" viewPort=\"0 0 $width $height\"";
		$this->svg .= ">\n";
	}
	/**
	 * @param int|float|null $r
	 * @param int|null $line_width
	 * @param string|bool|null $style
	 * @param string|bool|null $tooltip
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function circle($x, $y, $r, $bcolor=false, $line_width=1, $color=false, $style=false, $tooltip=false)
	{
		$this->svg .= "<circle cx=\"$x\" cy=\"$y\" r=\"$r\"";
		if (!empty($line_width))
			$this->svg .= " stroke-width=\"$line_width\"";
		if (!empty($bcolor))
			$this->svg .= " stroke=\"$bcolor\"";
		if (empty($color))
			$color = "none";
		$this->svg .= " fill=\"$color\"";
		if (!empty($style))
			$this->svg .= " $style";
		if (!empty($tooltip))
			$this->svg .= "><title>$tooltip</title></circle>\n";
		else
			$this->svg .= " />\n";
	}
	/**
	 * @param int|null $line_width
	 * @param string|bool|null $style
	 * @param string|bool|null $tooltip
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function rect($x1, $y1, $x2, $y2, $bcolor=false, $line_width=1, $color=false, $style=false, $tooltip=false)
	{
		$this->svg .= "<rect x=\"$x1\" y=\"$y1\" width=\"$x2\" height=\"$y2\"";
		if (!empty($line_width))
			$this->svg .= " stroke-width=\"$line_width\"";
		if (!empty($bcolor))
			$this->svg .= " stroke=\"$bcolor\"";
		if (empty($color))
			$color = "none";
		$this->svg .= " fill=\"$color\"";
		if (!empty($style))
			$this->svg .= " $style";
		if (!empty($tooltip))
			$this->svg .= "><title>$tooltip</title></rect>\n";
		else
			$this->svg .= " />\n";
	}
	/**
	 * @param int|null $line_width
	 * @param bool $color
	 * @param bool $style
	 * @param bool $tooltip
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function line($x1, $y1, $x2, $y2, $bcolor=false, $line_width=1, $color=false, $style=false, $tooltip=false)
	{
		$this->svg .= "<line x1=\"$x1\" y1=\"$y1\" x2=\"$x2\" y2=\"$y2\"";
		if (!empty($line_width))
			$this->svg .= " stroke-width=\"$line_width\"";
		if (!empty($bcolor))
			$this->svg .= " stroke=\"$bcolor\"";
		if (empty($color))
			$color = "none";
		$this->svg .= " fill=\"$color\"";
		if (!empty($style))
			$this->svg .= " $style";
		if (!empty($tooltip))
			$this->svg .= "><title>$tooltip</title></line>\n";
		else
			$this->svg .= " />\n";
	}
	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function polygon($p, $bcolor=false, $line_width=1, $color=false, $style=false, $tooltip=false)
	{
		$this->svg .= "<polygon points=\"$p\"";
		if (!empty($line_width))
			$this->svg .= " stroke-width=\"$line_width\"";
		if (!empty($bcolor))
			$this->svg .= " stroke=\"$bcolor\"";
		if (empty($color))
			$color = "none";
		$this->svg .= " fill=\"$color\"";
		if (!empty($style))
			$this->svg .= " $style";
		if (!empty($tooltip))
			$this->svg .= "><title>$tooltip</title></polygon>\n";
		else
			$this->svg .= " />\n";
	}
	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function polyline($p, $bcolor=false, $line_width=1, $color=false, $style=false, $tooltip=false)
	{
		$this->svg .= "<polyline points=\"$p\"";
		if (!empty($line_width))
			$this->svg .= " stroke-width=\"$line_width\"";
		if (!empty($bcolor))
			$this->svg .= " stroke=\"$bcolor\"";
		if (empty($color))
			$color = "none";
		$this->svg .= " fill=\"$color\"";
		if (!empty($style))
			$this->svg .= " $style";
		if (!empty($tooltip))
			$this->svg .= "><title>$tooltip</title></polyline>\n";
		else
			$this->svg .= " />\n";
	}
	/**
	 * @param string|null $d
	 * @param int|null $line_width
	 * @param string|bool|null $style
	 * @param string|bool|null $tooltip
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function path($d, $bcolor=false, $line_width=1, $color=false, $style=false, $tooltip=false)
	{
		$this->svg .= "<path d=\"$d\"";
		if (!empty($line_width))
			$this->svg .= " stroke-width=\"$line_width\"";
		if (!empty($bcolor))
			$this->svg .= " stroke=\"$bcolor\"";
		if (empty($color))
			$color = "none";
		$this->svg .= " fill=\"$color\"";
		if (!empty($style))
			$this->svg .= " $style";
		if (!empty($tooltip))
			$this->svg .= "><title>$tooltip</title></path>\n";
		else
			$this->svg .= " />\n";
	}
	/**
	 * @param string|bool|null $style
	 * @param string|bool|null $align
	 * @param int|bool|null $angle
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function text($x, $y, $text, $size=false, $color=false, $style=false, $align=false, $angle=false) // align start, middle, end. 
	{
		$this->svg .= "<text x=\"$x\" y=\"$y\"";
		if (!empty($color))
			$this->svg .= " fill=\"$color\"";
		if (!empty($size))
			$this->svg .= " font-size=\"$size\"";
		if (!empty($style))
			$this->svg .= " $style";
		if (!empty($align))
			$this->svg .= " text-anchor=\"$align\"";
		if (!empty($angle))
			$this->svg .= " transform=\"rotate($angle $x $y)\"";
		$this->svg .= ">$text</text>\n";
	}
	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function other($s)
	{
		$this->svg .= $s;
	}
	/**
	 * @param string|null $s
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function style($s='')
	{
		$this->svg .= "<defs><style type=\"text/css\"><![CDATA[ $s ]]></style></defs>\n";
	}
	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function open_group($p = '')
	{
		$this->svg .= "<g $p>\n";
	}
	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function close_group()
	{
		$this->svg .= "</g>\n";
	}
	/**
	 * @psalm-external-mutation-free
	 * @return void
	 */
	function close()
	{
		$this->svg .= "</svg>\n";
	}
	/** @return true */
	function draw()
	{
		//header('Content-Type: image/svg+xml');
		echo $this->svg;
		return true;
	}
	/** @return void */
	function save($fileName)
	{
		file_put_contents($fileName, $this->svg);
	}
}

// The following is for handling RTL texts. GD does not handle RTL at all.
// The function, arabic, has been taken from Milad Rastian and
// modified by Bagram Siadat.
// The function has been further modified and several bugs are fixed.

/** @psalm-pure */
function is_arabic(string|int|float|bool|null $text): int|false
{
	return preg_match('/\p{Arabic}/u', $text);
} 

/** @psalm-pure */
function is_hebrew(string|int|float|bool|null $text): int|false
{
	return preg_match('/\p{Hebrew}/u', $text);
} 

/** @psalm-pure */
function utf8_strlen(string|int|float|bool|null $str): int|false 
{
	return preg_match_all('/[\x00-\x7F\xC0-\xFD]/', $str, $dummy);
}

/** @psalm-pure */
function utf8_chr(?int $uni): string 
{
	$r = "";
	# ASCII range (including control chars)
	if ( ($uni >= 0) && ($uni <= 0x007f) ) 
			$r .= chr($uni);
	elseif ($uni <= 0x07ff) // 2 byte sequence
	{
		$r .= chr(0xc0 | ($uni >> 6));
		$r .= chr(0x80 | ($uni & 0x003f));
	} 
	elseif($uni == 0xFEFF) // Byte order mark (skip)
		return chr(0); // nop -- zap the BOM
	elseif ($uni >= 0xD800 && $uni <= 0xDFFF) // Test for illegal surrogates 
		return chr(0); // found a surrogate
	elseif ($uni <= 0xffff) // 3 byte sequence 
	{
		$r .= chr(0xe0 | ($uni >> 12));
		$r .= chr(0x80 | (($uni >> 6) & 0x003f));
		$r .= chr(0x80 | ($uni & 0x003f));
	} 
	elseif ($uni <= 0x10ffff) // 4 byte sequence 
	{
		$r .= chr(0xf0 | ($uni >> 18));
		$r .= chr(0x80 | (($uni >> 12) & 0x3f));
		$r .= chr(0x80 | (($uni >> 6) & 0x3f));
		$r .= chr(0x80 | ($uni & 0x3f));
	} 
	else 
		return chr(0);
	return $r;
}

///
/** @psalm-pure */
function arabic(string|int|float|bool|null $str,string|int|float|bool|null $z="",string|int|float|bool|null $method='normal'): string
{
$p_chars = array (
	utf8_chr(0x0622) => array (utf8_chr(0xfe82), utf8_chr(0xfe82), utf8_chr(0x0622)),
	utf8_chr(0x0627) => array (utf8_chr(0xfe8e), utf8_chr(0xfe8e), utf8_chr(0x0627)),
	utf8_chr(0x0628) => array (utf8_chr(0xfe90), utf8_chr(0xfe92), utf8_chr(0xfe91)),
	utf8_chr(0x067e) => array (utf8_chr(0xfb57), utf8_chr(0xfb59), utf8_chr(0xfb58)),
	utf8_chr(0x062a) => array (utf8_chr(0xfe96), utf8_chr(0xfe98), utf8_chr(0xfe97)),
	utf8_chr(0x062b) => array (utf8_chr(0xfe9a), utf8_chr(0xfe9c), utf8_chr(0xfe9b)),
	utf8_chr(0x062c) => array (utf8_chr(0xfe9e), utf8_chr(0xfea0), utf8_chr(0xfe9f)),
	utf8_chr(0x0686) => array (utf8_chr(0xfb7b), utf8_chr(0xfb7d), utf8_chr(0xfb7c)),
	utf8_chr(0x062d) => array (utf8_chr(0xfea2), utf8_chr(0xfea4), utf8_chr(0xfea3)),
	utf8_chr(0x062e) => array (utf8_chr(0xfea6), utf8_chr(0xfea8), utf8_chr(0xfea7)),
	utf8_chr(0x062f) => array (utf8_chr(0xfeaa), utf8_chr(0xfeaa), utf8_chr(0xfea9)),
	utf8_chr(0x0630) => array (utf8_chr(0xfeac), utf8_chr(0xfeac), utf8_chr(0xfeab)),
	utf8_chr(0x0631) => array (utf8_chr(0xfeae), utf8_chr(0xfeae), utf8_chr(0xfead)),
	utf8_chr(0x0632) => array (utf8_chr(0xfeb0), utf8_chr(0xfeb0), utf8_chr(0xfeaf)),
	utf8_chr(0x0698) => array (utf8_chr(0xfb8b), utf8_chr(0xfb8b), utf8_chr(0xfb8a)),
	utf8_chr(0x0633) => array (utf8_chr(0xfeb2), utf8_chr(0xfeb4), utf8_chr(0xfeb3)),
	utf8_chr(0x0634) => array (utf8_chr(0xfeb6), utf8_chr(0xfeb8), utf8_chr(0xfeb7)),
	utf8_chr(0x0635) => array (utf8_chr(0xfeba), utf8_chr(0xfebc), utf8_chr(0xfebb)),
	utf8_chr(0x0636) => array (utf8_chr(0xfebe), utf8_chr(0xfec0), utf8_chr(0xfebf)),
	utf8_chr(0x0637) => array (utf8_chr(0xfec2), utf8_chr(0xfec4), utf8_chr(0xfec3)),
	utf8_chr(0x0638) => array (utf8_chr(0xfec6), utf8_chr(0xfec8), utf8_chr(0xfec7)),
	utf8_chr(0x0639) => array (utf8_chr(0xfeca), utf8_chr(0xfecc), utf8_chr(0xfecb)),
	utf8_chr(0x063a) => array (utf8_chr(0xfece), utf8_chr(0xfed0), utf8_chr(0xfecf)),
	utf8_chr(0x0641) => array (utf8_chr(0xfed2), utf8_chr(0xfed4), utf8_chr(0xfed3)),
	utf8_chr(0x0642) => array (utf8_chr(0xfed6), utf8_chr(0xfed8), utf8_chr(0xfed7)),
	utf8_chr(0x06a9) => array (utf8_chr(0xfeda), utf8_chr(0xfedc), utf8_chr(0xfedb)),
	utf8_chr(0x06af) => array (utf8_chr(0xfb93), utf8_chr(0xfb95), utf8_chr(0xfb94)),
	utf8_chr(0x0644) => array (utf8_chr(0xfede), utf8_chr(0xfee0), utf8_chr(0xfedf)),
	utf8_chr(0x0645) => array (utf8_chr(0xfee2), utf8_chr(0xfee4), utf8_chr(0xfee3)),
	utf8_chr(0x0646) => array (utf8_chr(0xfee6), utf8_chr(0xfee8), utf8_chr(0xfee7)),
	utf8_chr(0x0648) => array (utf8_chr(0xfeee), utf8_chr(0xfeee), utf8_chr(0xfeed)),
	utf8_chr(0x06cc) => array (utf8_chr(0xfbfd), utf8_chr(0xfbff), utf8_chr(0xfbfe)),
	utf8_chr(0x0643) => array (utf8_chr(0xfeda), utf8_chr(0xfedc), utf8_chr(0xfedb)),
	utf8_chr(0x064a) => array (utf8_chr(0xfef2), utf8_chr(0xfef4), utf8_chr(0xfef3)),
	utf8_chr(0x0623) => array (utf8_chr(0xfe84), utf8_chr(0xfe84), utf8_chr(0xfe83)),
	utf8_chr(0x0624) => array (utf8_chr(0xfe86), utf8_chr(0xfe86), utf8_chr(0xfe85)),
	utf8_chr(0x0625) => array (utf8_chr(0xfe88), utf8_chr(0xfe88), utf8_chr(0xfe87)),
	utf8_chr(0x0626) => array (utf8_chr(0xfe8a), utf8_chr(0xfe8c), utf8_chr(0xfe8b)),
	utf8_chr(0x0629) => array (utf8_chr(0xfe94), utf8_chr(0xfe98), utf8_chr(0xfe97))
);

$nastaligh = array (
	utf8_chr(0x0647) => array (utf8_chr(0xfbab), utf8_chr(0xfbad), utf8_chr(0xfbac))
);

$normal = array (
	utf8_chr(0x0647) => array (utf8_chr(0xfeea), utf8_chr(0xfeec), utf8_chr(0xfeeb))
);

$mp_chars = array (utf8_chr(0x0622), utf8_chr(0x0627), utf8_chr(0x062f), utf8_chr(0x0630), utf8_chr(0x0631), utf8_chr(0x0632), 
	utf8_chr(0x0698), utf8_chr(0x0648), utf8_chr(0x0623), utf8_chr(0x0625), utf8_chr(0x0624));

$ignorelist = array (utf8_chr(0x0000), utf8_chr(0x064c), utf8_chr(0x064d), utf8_chr(0x064b), utf8_chr(0x064f), utf8_chr(0x0650), 
	utf8_chr(0x064e), utf8_chr(0x0651), utf8_chr(0x0653), utf8_chr(0x0670), utf8_chr(0x0654), utf8_chr(0xfe76), utf8_chr(0xfe7a), 
	utf8_chr(0xfe78), utf8_chr(0xfe7c), utf8_chr(0xfe7e), utf8_chr(0xfe74), utf8_chr(0xfe70), utf8_chr(0xfc5e), utf8_chr(0xfc5f), 
	utf8_chr(0xfc60), utf8_chr(0xfc61), utf8_chr(0xfc62), utf8_chr(0xfc63));

	$str_back = $output = $e_output = $str_next = $str1 = $num = "";
	if ($method == 'nastaligh')
		$p_chars = array_merge($p_chars, $nastaligh);
	else
		$p_chars = array_merge($p_chars, $normal);
	$str_len = utf8_strlen($str);
	preg_match_all("/./u", $str, $ar);
	for ($i = 0; $i < $str_len; $i++)
	{
		if (isset($ar[0][$i]))
			$str1 = $ar[0][$i];
		if(isset($ar[0][$i+1]) && in_array($ar[0][$i+1], $ignorelist))
		{
			if (isset($ar[0][$i+2]))
				$str_next = $ar[0][$i+2];
			if ($i == 2) 
				$str_back = $ar[0][$i-2];
			if ($i > 1 && $i != 2) 
				$str_back = $ar[0][$i-1];
		}
		elseif ($i > 0 && isset($ar[0][$i-1]) && !in_array($ar[0][$i-1], $ignorelist))
		{
			if (isset($ar[0][$i+1]))
				$str_next = $ar[0][$i+1];
			if ($i != 0) 
				$str_back = $ar[0][$i-1];
		}
		else
		{
			if (isset($ar[0][$i+1]) && (bool)($ar[0][$i+1] ?? null))
				$str_next = $ar[0][$i+1];
			elseif ($i > 0 && isset($ar[0][$i-1])) 
				$str_next = $ar[0][$i-1];
			if ($i > 1 && isset($ar[0][$i-2])) 
				$str_back = $ar[0][$i-2];
		}
		if (!in_array($str1,$ignorelist))
		{
			if (array_key_exists($str1,$p_chars))
			{
				if (!(bool)$str_back || $str_back==" " || !array_key_exists($str_back,$p_chars))
				{
					if (!array_key_exists($str_back, $p_chars) && !array_key_exists($str_next, $p_chars)) 
						$output = $str1.$output;
					else 
						$output = $p_chars[$str1][2].$output;
					continue;
				}
				elseif (array_key_exists($str_next, $p_chars) && array_key_exists($str_back, $p_chars))
				{
					if (in_array($str_back, $mp_chars) && array_key_exists($str_next, $p_chars))
						$output = $p_chars[$str1][2].$output;
					else
						$output = $p_chars[$str1][1].$output;
					continue;
				}
				elseif (array_key_exists($str_back, $p_chars) && !array_key_exists($str_next, $p_chars))
				{
					if (in_array($str_back, $mp_chars))
						$output = $str1.$output;
					else
						$output = $p_chars[$str1][0].$output;
					continue;
				}
			}
			elseif ($z == "we")
			{
				$number = array('0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
					"\xD9\xA0", "\xD9\xA1", "\xD9\xA2", "\xD9\xA3", "\xD9\xA4", "\xD9\xA5", "\xD9\xA6", 
					"\xD9\xA7", "\xD9\xA8", "\xD9\xA9",	"\xDB\xB0", "\xDB\xB1", "\xDB\xB2", "\xDB\xB3", 
					"\xDB\xB4", "\xDB\xB5", "\xDB\xB6", "\xDB\xB7", "\xDB\xB8", "\xDB\xB9");
				if (in_array($str1, $number))
				{
					$num .= $str1;
					$str1 = "";
				}
				if (!in_array($str_next, $number))
				{
					$str1 .= $num;
					$num = "";
				}
				//$output = $str1.$output;
				$output = $output.$str1;
			}
			elseif ($z == "fa")
			{
				$number = array (utf8_chr(0x0660), utf8_chr(0x0661), utf8_chr(0x0662), 
					utf8_chr(0x0663), utf8_chr(0x0664), utf8_chr(0x0665), utf8_chr(0x0666), 
					utf8_chr(0x0667), utf8_chr(0x0668), utf8_chr(0x0669), utf8_chr(0x06f4), 
					utf8_chr(0x06f5), utf8_chr(0x06f6), utf8_chr(0x0030), utf8_chr(0x0031), 
					utf8_chr(0x0032), utf8_chr(0x0033), utf8_chr(0x0034), utf8_chr(0x0035), 
					utf8_chr(0x0036), utf8_chr(0x0037), utf8_chr(0x0038), utf8_chr(0x0039));
				switch ($str1)
				{
					case ")" : 
						$str1 = "("; 
						break;
					case "(" : 
						$str1 = ")"; 
						break;
					case "}" : 
						$str1 = "{"; 
						break;
					case "{" : 
						$str1 = "}"; 
						break;
					case "]" : 
						$str1 = "["; 
						break;
					case "[" : 
						$str1 = "]"; 
						break;
					case ">" : 
						$str1 = "<"; 
						break;
					case "<" : 
						$str1 = ">"; 
						break;
				}
				if (in_array($str1, $number))
				{
					$num .= $str1;
					$str1 = "";
				}
				if (!in_array($str_next, $number))
				{
					$str1 .= $num;
					$num = "";
				}
				$output = $str1.$output;
			}
			else
			{
				if (($str1 == utf8_chr(0x060c)) || ($str1 == utf8_chr(0x061f)) || ($str1 == utf8_chr(0x0621)) || 
					(array_key_exists($str_next, $p_chars) && array_key_exists($str_back, $p_chars)) || 
					($str1 == " " && array_key_exists($str_back, $p_chars)) || ($str1 == " " && 
					array_key_exists($str_next, $p_chars)))
				{
					if ($e_output)
					{
						$output = $e_output.$output;
						$e_output = "";
					}
					$output = $str1.$output;
				}
				else
				{
					$e_output .= $str1;
					if (array_key_exists($str_next, $p_chars) || $str_next == "")
					{
						$output = $e_output.$output;
						$e_output = "";
					}
				}
			}
		}
		else
			$output = $str1.$output;
		$str_next = null;
		$str_back = null;
	}
	return	$output;
}

// For debugging
if (!function_exists('dump'))
{
	function dump(string|int|float|bool|null $str): void
	{
		$req_dump = print_r($str, true);
		$fp = fopen('dump.log', 'a');
		date_default_timezone_set("Europe/Amsterdam");
		$msg = "[" . date('Y-m-d / H:i:s') . "] - $req_dump\n";
		fwrite($fp, $msg);
		fclose($fp);
	}
}
