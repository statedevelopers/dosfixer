<?php
/**
 * @file
 * Functions for manipulating data within various entities. 
 *
 */
 
 /** ***************************************************************************
 * Array gathering functions.
 *
 */
 
  /**
   * Loads Regions vocabulary as objects.
   *
   */
    function dosfixer_loadregions() {
      // LOAD REGIONAL TERMS
      $regions = array();
      $result = db_query("SELECT tid FROM {taxonomy_term_data} WHERE vid = 18;");
      foreach ($result as $record) {
        $regions[] = taxonomy_term_load($record->tid);
      }
      
      return($regions);
    }
  
  /**
   * Loads users as objects.
   *
   */
    function dosfixer_loadusers() {
      // LOAD USERS
      $users = array();
      $result = db_query("SELECT uid FROM {users};");
      foreach ($result as $record) {
        $users[] = user_load($record->uid);
      }
      
      return($users);
    }
  
  /**
   * Loads Countries vocabulary as strings, keyed by tid.
   *
   */
    function dosfixer_loadcountries() {
      // LOAD COUNTRY TERMS
      $countries = array();
      $result = db_query("SELECT tid, name FROM {taxonomy_term_data} WHERE vid = 16;");
      foreach ($result as $record) {
        $countries[$record->tid] = $record->name;
      }
      
      return($countries);
    }

  /**
   * Loads Countries vocabulary into array keyed by tid, with values of Region by tid.
   *
   */
    function dosfixer_loadcountryregions() {
      // LOAD COUNTRY Parents
      $countryregions = array();
        $query  = "SELECT h.tid as tid, t.name as country, r.name as region, r.tid as rtid ";
        $query .= "FROM d7_gallery.taxonomy_term_hierarchy h ";
        $query .= "INNER JOIN d7_gallery.taxonomy_term_data t ";
        $query .= "  ON t.tid = h.tid ";
        $query .= "INNER JOIN d7_gallery.taxonomy_term_data p "; 
        $query .= "  ON p.tid = h.parent ";
        $query .= "INNER JOIN d7_gallery.taxonomy_term_data r "; 
        $query .= "  ON r.name = p.name ";
        $query .= "WHERE t.vid = 16 ";
        $query .= "  AND r.vid = 18 ";
        $query .= "ORDER BY r.name, t.name; ";

      $result = db_query($query);
      foreach($result as $record) {
        $countryregions[$record->tid] = $record->rtid;
      }
      
      return($countryregions);
    }
  
  
 /** ***************************************************************************
 * Node editing functions.
 *
 */
 
  /**
   * SET Country from Title and Description
   *
   */
    function dosfixer_setnode_countryfromcontent($node, $countries, $changecount = 0) {
      if($node->field_country['und'][0]['tid']!= 1566) { // Set to default 'U.S.'
        $country = taxonomy_term_load($node->field_country['und'][0]['tid']);
print "Node <a href=\"/node/$node->nid\">$node->nid</a> has country '$country->name'.<br />\n";
        $title = $node->title;
        $desc = $node->field_description['und'][0]['value'];
        
        $title = str_ireplace('Myanmar', 'Burma', $title);
        $desc = str_ireplace('Myanmar', 'Burma', $desc);
        
        // Step through countries to find a match
        foreach($countries as $tid=>$country) {
          if($country=='United States') continue;
          if($country=='Africa (sub Sahara)') continue;
          if($country=='Domestic') continue;
          if($country=='International') continue;
          
          if(dosfixer_strpos($title,$country,1)!==FALSE || dosfixer_strpos($desc,$country,1)!==FALSE) {            
            // False Positives
            if($country=='Spain') {
              if(
                dosfixer_strpos($title,"Port of Spain",1)!==FALSE 
                || dosfixer_strpos($desc,"Port of Spain",1)!==FALSE
                || dosfixer_strpos($title,"Port-of-Spain",1)!==FALSE 
                || dosfixer_strpos($desc,"Port-of-Spain",1)!==FALSE
              ) {
                continue;
              }            
            }
          
            if($node->field_country['und'][0]['tid'] != $tid) {
print "-->Node <a href=\"/node/$node->nid\">$node->nid</a>: Key $key: Changing country to: '$country' (tid:$tid).<br />\n";
              $node->field_country['und'][0]['tid'] = $tid;
              node_save($node); $changecount++; // Don't use node_submit, which changes Author.
              break;
            }
          }         
        }
      }
    }
  
  /**
   * SET AUTHOR from field_auxauthor
   *
   */
    function dosfixer_setnode_authorfromauxauthor($node) {
      // Must have auxiliary author field value && must not be assigned to any user other than Admin
      if(!empty($node->field_auxauthor['und'][0]['value']) && $node->uid == 1) {
        // If not admin, then verify the username. 
        $result = db_query("SELECT u.name FROM users u WHERE u.uid = " . $node->uid);
        
        $auxauthor = $node->field_auxauthor['und'][0]['value'];
        // Exceptions for founding editors
        if($auxauthor == 'admin') $auxauthor = 'JonesEM'; // Eric Jones was the previous admin.
        if($auxauthor == 'lforgerson') $auxauthor = 'ForgersonLA'; 
        if($auxauthor == 'cjjackson') $auxauthor = 'JacksonJ2'; 
        if($auxauthor == 'Ruth Bennett') $auxauthor = 'BennettRE'; 
        if($auxauthor == 'Aaron Bruce') $auxauthor = 'abruce'; 
        if($auxauthor == 'PerryMD') $auxauthor = 'PerryMD2'; 
        if($auxauthor == 'adamss') $auxauthor = 'adamssc'; 
        if($auxauthor == 'johnsonha') $auxauthor = 'johnsonhm';
        
        // Verify a real user
        if($user = db_query('SELECT uid FROM {users} WHERE name = :name', array(':name' => $auxauthor))->fetchObject()) {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Changing author to: " . $user->uid . " -> '$auxauthor'.<br />\n";
          $node->uid = $user->uid;
          node_save($node); return TRUE; // Don't use node_submit, which changes Author. 
        } else {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: No user by the name of '$auxauthor'.<br />\n";
        }
      } else {
        if(empty($node->field_auxauthor['und'][0]['value'])) {
//print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: No auxiliary author data.<br />\n";
        } elseif ($node->uid != 1) {
//print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Node is already assigned to user: $node->uid.<br />\n";
        }
      }
      
      return FALSE;
    }
  
  /**
   * SET BODY SUMMARY from field_auxiliary_summary
   *
   */
    function dosfixer_setnode_restoresummary($node) {
      // Must have auxiliary summary field value && true summary field must not be occupied. 
      if(!empty($node->field_auxiliary_summary['und'][0]['value']) && empty($node->body['und'][0]['summary'])) {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Changing summary to: " . substr($node->field_auxiliary_summary['und'][0]['value'], 0, 90) . "..." . ".<br />\n";
        $node->body['und'][0]['summary'] = $node->field_auxiliary_summary['und'][0]['value'];
        node_save($node); return TRUE; // Don't use node_submit, which changes Author. 
      } else {
        if(empty($node->field_auxiliary_summary['und'][0]['value'])) {
//print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: No auxiliary summary data.<br />\n";
        } elseif (!empty($node->body['und'][0]['summary'])) {
//print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Summary is already set.<br />\n";
        }
      }
      
      return FALSE;
    }
  
  /**
   * SET BRIGHTCOVE VIDEO from field_data_field_auxiliary_bcvideo
   *
   * NOTE: Uses BrightCove's suite of modules, which ultimately takes only the ID of the video
   * that is stored in their system.
   *
   */
    function dosfixer_setnode_setbcvideo($node) {
      // Must have auxiliary video field value && video field must be empty. 
      if(!empty($node->field_auxiliary_bcvideo['und'][0]['value']) && empty($node->field_brightcove_video['und'][0]['video_id'])) {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Filling BC video with " . $node->field_auxiliary_bcvideo['und'][0]['value'] . ".<br />\n";
        $node->field_brightcove_video['und'][0]['video_id'] = $node->field_auxiliary_bcvideo['und'][0]['value'];
        node_save($node); return TRUE; // Don't use node_submit, which changes Author. 
      } else {
        if(empty($node->field_auxiliary_bcvideo['und'][0]['value'])) {
//print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: No auxiliary BC video data.<br />\n";
        } elseif (!empty($node->field_brightcove_video['und'][0]['video_id'])) {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: BC Video is already set.<br />\n";
        }
      }
      
      return FALSE;
    }
    
  /**
   * SET EMBEDDED VIDEO from field_auxiliary_video
   *
   * NOTE: Uses Phase 2's Video Embed module, which takes only a URL field, and the rest is automatically filled. 
   * This strips embed code to just the URL and then inserts into the field's video_url value.
   *
   */
    function dosfixer_setnode_setauxvideo($node) {
      // Must have auxiliary video field value && video field must be empty. 
      if(!empty($node->field_auxiliary_video['und'][0]['value']) && empty($node->field_video['und'][0]['video_url'])) {
        if ($url = dosfixer_extractsrc($node->field_auxiliary_video['und'][0]['value'])) {
  print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Filling video with: " . htmlentities(substr($url, 0, 90)) . "..." . ".<br />\n";
          $node->field_video['und'][0]['video_url'] = $url;
          node_save($node); return TRUE; // Don't use node_submit, which changes Author. 
        } else {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Could not extract URL form embed code '" . $node->field_auxiliary_video['und'][0]['value'] . "'.<br />\n";
        }
      } else {
        if(empty($node->field_auxiliary_video['und'][0]['value'])) {
//print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: No auxiliary video data.<br />\n";
        } elseif (!empty($node->field_video['und'][0]['video_url'])) {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Video is already set.<br />\n";
        }
      }
      
      return FALSE;
    }
    
    /**
     * EXTRACT SRC from HTML tag.
     *
     * Useful for video embed code.
     *
     */
      function dosfixer_extractsrc($html) {
        $args = explode(' ', $html);
        foreach($args as $arg) {
          if (strcasecmp(substr($arg, 0, 4), "src=")==0) {
            $src = substr($arg,4);
            $src = str_ireplace("\"", "", $src);
            $src = str_ireplace("\'", "", $src);
            return $src;
          }
        }
        
        return FALSE;
      }
    
  
  /**
   * SET IMAGE node reference from field_field_aux_imageref
   *
   */
    function dosfixer_setnode_setauximage($node) {
      // Must have auxiliary photo reference value && photo field node reference must not be set. 
      if(!empty($node->field_field_aux_imageref['und'][0]['value']) && empty($node->field_photo['und'][0]['target_id'])) {
        $results = db_query(
          "SELECT n.nid " . 
          "FROM {node} n " . 
          "INNER JOIN field_data_field_auxid aid ON aid.entity_id = n.nid " . 
          "WHERE n.type = 'image_record' " . 
          "AND aid.bundle = 'image_record' " . 
          "AND aid.field_auxid_value = :auxref;"
          ,array(":auxref" => $node->field_field_aux_imageref['und'][0]['value'])
        );
        foreach($results as $result) $photo_id = $result->nid;
  
        $photo = node_load($photo_id);
        // Check for valid photo.
        if($photo->nid) {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Match photo: <strong>'" . substr($photo->title,0,30) . "...'</strong> to article: <strong>'" . substr($node->title,0,30) . "...'</strong>.<br />\n";
          $node->field_photo['und'][0]['target_id'] = $photo->nid;
          node_save($node); return TRUE; // Don't use node_submit, which changes Author. 
        } else {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: AuxID '" . $node->field_auxid['und'][0]['value'] . "' matches no photo for ref. ID '" . $node->field_field_aux_imageref['und'][0]['value'] . "'.<br />\n";
        }
      } else {
        if(empty($node->field_field_aux_imageref['und'][0]['value'])) {
//print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: No photo reference.<br />\n";
        } elseif (!empty($node->field_photo['und'][0]['target_id'])) {
//print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Photo is already set.<br />\n";
        }
      }
      
      return FALSE;
    }

  /**
   * SET REGION, based on keywords
   *
   */
    function dosfixer_setnode_regionfromkeywords($node, $regions, $changecount = 0) {
      // Look for empty region, but with keywords
      if(empty($node->field_region['und'][0]['tid']) && !empty($node->field_keywords)) {
        $keywords = $node->field_keywords['und'][0]['value'];

        // SET REGION, based on keywords ---------------------------------------
        // Look for empty region, but with keywords
        if(empty($node->field_region['und'][0]['tid'])) { // && !empty($node->field_keywords)) {
          $country = taxonomy_term_load($node->field_country['und'][0]['tid']);
          $keywords = $node->field_keywords['und'][0]['value'];
          
          foreach($regions as $region) {
            $set_region = FALSE;
  //print "-->$region->tid: Compare: " . $keywords . " | " . $region->name . "<br />\n";
            if(stripos($keywords,$region->name)!==FALSE) {
              $set_region = TRUE;
  //print "-->MATCH: $region->tid: Compare: " . $keywords . " | " . $region->name . "<br />\n";
            } elseif(!empty($region->synonyms_synonym['und'])) {
              foreach($region->synonyms_synonym['und'] as $synonym) {
  //print "-->$region->tid: Compare synonym: " . $keywords . " | " . $synonym['value'] . "<br />\n";
                if (stripos($keywords,$synonym['value'])!==FALSE) {
                  $set_region = TRUE;
  //print "-->MATCH: $region->tid: Compare synonym: " . $keywords . " | " . $synonym['value'] . "<br />\n";              
                  break;
                }
              }
            }
            if($set_region) {
              if(!is_array($node->field_region['und']))$node->field_region['und'] = array();
             //if(!is_array($node->field_region['und'][0]))$node->field_region['und'][0] = array();
              $node->field_region['und'][0]['tid'] = $region->tid;
  print "$key: Setting region to: " . $node->field_region['und'][0]['tid'] . " -> '$region->name' on node '$node->nid'.<br />\n";
              node_save($node); $changecount++;
              break;          
            }
          }
        }
      }
    }

  /**
   * SET REGION, based on Country field
   *
   */
    function dosfixer_setnode_regionfromcountry($node, $countryregions, $changecount = 0) {
      $regionfield = &$node->field_region['und'][0]['tid'];
      $countryfield = &$node->field_country['und'][0]['tid'];

      $country = taxonomy_term_load($countryfield);
      $region0 = taxonomy_term_load($regionfield);

      // Look for empty region, but with country that is NOT U.S., nor Africa (region), nor the Moon
      if(empty($regionfield)) {        
        // Detect conflicts
        $confirmed = FALSE;
        foreach($node->field_region['und'] as $regionarry) {
          if($regionarray['tid'] == $countryregions[$countryfield]) $confirmed = TRUE;
        }
        if($regionfield != $countryregions[$countryfield]) {
          $region1 = taxonomy_term_load($countryregions[$countryfield]);
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>; CONFLICT $country->name -> <strong>$region0->name | $region1->name</strong><br />";
        }
        
        $regionfield = $countryregions[$countryfield];
        if(!empty($regionfield)) {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>; Setting $countryfield -> Region <strong>$regionfield</strong><br />";
          //$node->field_region['und'][0]['tid'] = $region->tid;
          node_save($node); $changecount++;
        }
      } else {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>; EMPTY $country->name -> <strong>No Region</strong><br />";
      }
    }

  /**
   * SET Creation, based on Date field
   *
   */
    function dosfixer_setnode_creationfromdate($node, $changecount = 0) {
      $datefield = &$node->field_imagedate['und'][0]['value'];
      $date = strtotime($datefield);
      if($node->created > (strtotime('3/20/2012')-86400)) {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Created " . date('c',$node->created);
        $node->created = $date;
        node_save($node); $changecount++;
print ", changed to <strong>" . date('c',$node->created) . "</strong><br />\n";
      } else {
//print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Created " . date('c',$node->created) . "<br />\n";
      }
    }

  /**
   * SET Date, based on notes in description.
   *
   */
    function dosfixer_setnode_datefromdesc($node, $changecount = 0) {
      $datefield = &$node->field_imagedate['und'][0]['value'];
      $desc = $node->field_description['und'][0]['value'];
//print "Desc = '$desc'<br />\n";
      $monthArray = array('January','February','March','April','May','June','July','August','September','October','November','December');
      $moArray = array('Jan.','Feb.','Mar.','Apr.','May','Jun.','Jul.','Aug.','Sep.','Oct.','Nov.','Dec.');
      
      // Scan for month name to get date.
      foreach($monthArray as $monkey=>$month) {
//print "Month '$month' and '" . $moArray[$monkey] . "'<br />\n";
        $monthpos = dosfixer_strpos($desc, $month, 1);
        $mopos = dosfixer_strpos($desc, $moArray[$monkey], 1);
        if($monthpos!==FALSE) {
          $pos = $monthpos;
        } elseif($mopos!==FALSE) {
          $pos = $mopos;
        } else {
          continue;
        }

        // Process
        $data = substr($desc, $pos);
        $data = str_replace('.','. ',$data);
        $data = str_replace(',','',$data);
        $data = str_replace('  ',' ',$data);
        $data = str_replace('  ',' ',$data);
        $data = str_replace('  ',' ',$data);
//print "Data: '$data' <br />\n";
        $dataArray = explode(' ',$data);
        $dateArray = array();
        for($i=0; $i<3; $i++) $dateArray[] = array_shift($dataArray);
//print "DateArray: '" . implode(' ',$dateArray) . "' <br />\n";
        
        // Qualify
        if(count($dateArray) != 3) continue;
//print "Flag a. Checking day '" . $dateArray[1] . "'<br />\n";
        if(!ctype_digit($dateArray[1])) continue;
//print "Flag b.<br />\n";
        if($dateArray[1] < 1 || $dateArray[1] > 31) continue;
        $dateArray[2] = str_replace('.','',$dateArray[2]);
//print "Flag c. Checking year '" . $dateArray[2] . "'<br />\n";       
        if(!ctype_digit($dateArray[2])) continue;
//print "Flag d.<br />\n";
        if($dateArray[2] < 1775 || $dateArray[2] > intval(date('Y'))) continue;
//print "Flag e: " . $dateArray[0] . " " . $dateArray[1] . ", " . $dateArray[2] . ".<br />\n";
        
        // Convert
        $date = strtotime($dateArray[0] . " " . $dateArray[1] . ", " . $dateArray[2]);
        
        // Verify conversion
        if($date < strtotime('1/1/1970') || $date > time()) continue;
        
        // Measure significance of delta
          // First convert field to integer, if not already. Date field cannot be saved in other formats without special functions. 
          $test = strtotime($datefield);
          if(!empty($test)) $datefield = $test;
        $diff = $datefield - $date;
        if($diff >= 86400) {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: RESETTING DATE to '" . substr(date('c',$date),0,-6) . ", " . intval($diff/86400) . " days earlier than '" . substr(date('c',$datefield),0,-6) . "'.<br />\n";            
          $datefield = substr(date('c',$date),0,-6); 
          node_save($node); $changecount++;
          break;
//        } else {
//print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: '" . date('c',$datefield) . "', DIFF: " . number_format($diff/3600,2) . " hours.<br />\n";            
        }
      }
    }

  /**
   * SET Date, based on exif metadata in the photo files.
   *
   */
    function dosfixer_setnode_datefromexif($node, $changecount = 0) {
      $filename = $node->field_image['und'][0]['filename'];
      $path = '/var/www/html/sites/default/files/contributed_images';
      $filepath = $path . '/' . $filename;
      if(!file_exists($filepath)) {
        print "ERROR: File '$filename' cannot be found at '$filepath'.<br />\n";
        exit;
      }
      $exif = exif_read_data($filepath);
      if($exif !== FALSE) {
        $exifdate = $exif['FileDateTime'];
        $diff = $node->created - $exifdate; 
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Exif date '" . date('c',$exifdate) . ", DIFF: '$diff'.<br />\n";
        
        // Take the earlier date. 
        if($diff >= 86400) {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: RESETTING TO Exif date '" . date('c',$exifdate) . ", from '" . date('c',$node->created) . "'.<br />\n";
          $node->created = $exifdate; 
//          node_save($node); $changecount++;
        }
      } else {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: No exif metadata available.<br />\n";
      }
    }

  /**
   * SET persons, based title, description.
   *
   */
    function dosfixer_setnode_personfromcontent($node, $changecount = 0) {
      //$synonymArray = array('Hillary Rodham Clinton','Hillary Clinton','Secretary Clinton','Secretary of State Clinton');
      //$prime_tid = 1309;
      $synonymArray = array('Barack Obama','President Obama','Mr. Obama');
      $prime_tid = 1831;
      
      if(!is_array($node->field_persons)) $node->field_persons = array();
      if(count($node->field_persons)<1) $node->field_persons['und'] = array();
      
      $personfield = &$node->field_persons['und'];
      $content = $node->title . $node->field_description['und'][0]['value']; // Concat to save time.
      
      
      // Stop if term is already assigned.
      foreach($personfield as $term) {
        if($term['tid'] == $prime_tid) return;
      }
      
      
      /*
      // Kill spare terms.
      $found = FALSE;
      foreach($personfield as $pkey=>$term) {
        if($term['tid'] == $prime_tid) {
          if($found) {
            unset($personfield[$pkey]);
            $killed = TRUE;
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Killing.<br />\n";
          } else {
            $found = TRUE;
          }
        }
      }
      if($killed) {
        node_save($node); $changecount++;
      }

      return;
      */
      

      // Scan for occurences of any synonym.
      foreach($synonymArray as $key=>$name) {
        if(dosfixer_strpos($content, $name, 0)!==FALSE) {
          
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Add term to '" . substr($content,dosfixer_strpos($content, $name, 0), 40) . "...<br />\n";
          $personfield[] = array('tid'=>$prime_tid);
          node_save($node); $changecount++;
          break;
        }
      }
    }


  /**
   * SET persons, based title, description.
   *
   */
    function dosfixer_setnode_dateformat($node, $changecount = 0) {
      $olddate = $node->field_imagedate['und'][0]['value'];
      if(ctype_digit($olddate)) {
        $node->field_imagedate['und'][0]['value'] = substr(date('c',$olddate),0,-6);
        $imagedatefield = $node->field_imagedate['und'][0]['value'];
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: ImageDate: '$olddate', change to '$imagedatefield'.<br>\n";
        node_save($node); $changecount++;
//      } else {
//print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Old date '$olddate' is ok.<br>\n";
      }
    }

  /**
   * .
   *
   */
    function dosfixer_shownode($nid0, $nid1 = 0) {
        $nodes = array();
        $nodes[] = node_load($nid0); 
        if($nid1) {
          $nodes[] = node_load($nid1); 
        }
        
        foreach($nodes as $node) {
          print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: ";
          print "ImageDate: '" . $node->field_imagedate['und'][0]['value'] . "'";
          print "<br>\n";
        }
        
        return(TRUE);
    }

  /**
   * Parse name into fullname (3 parts).
   *
   */
    function dosfixer_setuser_fullname($user, $changecount = 0) {
      if(!empty($user->field_auxname['und'])) 
        $auxname = $user->field_auxname['und'][0]['value'];

      if(!empty($auxname)) {
        if(empty($user->field_fullname['und'][0]['family'])) {
          // Truncate multiple names separated by commas
          $generational = "";
          if (stripos($auxname, ',')) {
            // Exception for Jr.'s
            if (stripos($auxname, ', jr')) {
              $generational = "Jr.";
            }
            $auxname = substr($auxname,0,stripos($auxname, ','));
          }
          
          // Truncate multiple names separated by 'and'
          if (stripos($auxname, ' and ')) {
            $auxname = substr($auxname,0,stripos($auxname, ' and '));
          }
          
          $namearray = explode(' ',$auxname);
          
          // Recombine Family name prefixes, like 'von'
          while($part = current($namearray)) {
            if(strcasecmp($part, 'von') == 0) {
              $part .= " " . next($namearray);
            }
            $newarray[] = $part;
            next($namearray);
          }
          $namearray = $newarray;
          
          // Load name parts
          $family = array_pop($namearray);
          $given = array_shift($namearray);
          $middle = array_shift($namearray);
          $user->field_fullname['und'][0]['given'] = $given;
          $user->field_fullname['und'][0]['safe']['given'] = $given;
          $user->field_fullname['und'][0]['middle'] = $middle;
          $user->field_fullname['und'][0]['safe']['middle'] = $middle;
          $user->field_fullname['und'][0]['family'] = $family;
          $user->field_fullname['und'][0]['safe']['family'] = $family;        
          $user->field_fullname['und'][0]['generational'] = $generational;
          $user->field_fullname['und'][0]['safe']['generational'] = $generational;        
  
          $user->field_display_name['und'][0]['value'] = $user->field_auxname['und'][0]['value'];        
          
print "user: <a href=\"/user/$user->uid/edit\">$user->uid</a>: Fullname: '" . implode('|',$user->field_fullname['und'][0]['safe']) . "' from '" . $user->field_display_name['und'][0]['value'] . "'.<br>\n";
        } 
        if (empty($user->field_display_name['und'][0]['value'])) {
          $user->field_display_name['und'][0]['value'] = $user->field_auxname['und'][0]['value'];        
print "user: <a href=\"/user/$user->uid/edit\">$user->uid</a>: SET Display name: '" . $user->field_display_name['und'][0]['value'] . "'.<br>\n";
        } else {
print "user: <a href=\"/user/$user->uid/edit\">$user->uid</a>: Already SET Display name: '" . $user->field_display_name['und'][0]['value'] . "'.<br>\n";
        } 
        //user_save($user); $changecount++;
      }
    }

  /**
   * SET CREATED & MODIFIED date fields from field_auxiliary_created & field_auxiliary_modified.
   *
   */
    function dosfixer_setnode_datesfromaux($node) {
      // Must have auxiliary video field value && video field must be empty.
      if(!empty($node->field_auxiliary_created['und'][0]['value'])) {
        $date = strtotime($node->field_auxiliary_created['und'][0]['value']);
        if(($node->created - $date) > 86400) {
          $node->created = $date;
          $node->changed = strtotime($node->field_auxiliary_modified['und'][0]['value']);
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Created " . date('c',$node->created) . "<br />\n";
          node_save($node); return TRUE; // Don't use node_submit, which changes Author.
        } else {
//print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Creation date is already set.<br />\n";
        }
      } else {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: No auxiliary creation date found.<br />\n";
      }

      return FALSE;
    }

  /**
   * Split tags improperly concatenated into a single term.
   * Change tags term field to remove the quotes surrounding the value.
   * Then register the tags separately.
   *
   */
    function dosfixer_setnode_splittags($node) {
      // Must have a tag field, && must be an array.
      if(!empty($node->field_tags['und'])) {
        if(is_array($node->field_tags['und'])) {
          // Survey the terms
          $NEEDS_SPLIT = FALSE;
          $termStrs = array();
          foreach($node->field_tags['und'] as $key=>$termArray) {
            $term = taxonomy_term_load($termArray['tid']);
            if(strpos($term->name, ',')) $NEEDS_SPLIT = TRUE;
            $termStrs = array_merge($termStrs, explode(",", str_replace("\"", "", $term->name))); // Remember to trim leading spaces
          }
          
          // Act only if terms appear merged. 
          if($NEEDS_SPLIT) {
            // Register new terms
            $newterms = array();
            $vocabulary = taxonomy_vocabulary_load(1);// 1 = tags, the default taxonomy vocabulary.
            foreach($termStrs as $termStr) {
              $newterms[] = dosfixer_setTerm(trim($termStr), $vocabulary);
            }
  
            // Replace node's terms
            $node->field_tags['und'] = array();
            foreach($newterms as $term) {
              $node->field_tags['und'][] = array('tid'=>$term->tid);
            }
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Tags split into <em>" . implode(' | ', $termStrs) . "</em><br />\n";
            node_save($node); return TRUE; // Don't use node_submit, which changes Author.
          } else {
//print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: NO MERGED terms present.<br />\n";
          }
        } else {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: NO TAGS to work with.<br />\n";
        }
      } else {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: EMPTY tag field.<br />\n";
      }

      return FALSE;
    }
    
  /**
   * Instantiate a NEW term. If it is not already registered, then save it. 
   *
   */
    function dosfixer_setTerm($name, $vocabulary) {
      $term = new stdClass();
      $term->name = $name;
      $term->vid = $vocabulary->vid;

      $qresult = db_query("SELECT tid FROM {taxonomy_term_data} WHERE vid = :vid AND name = :name ORDER BY tid;", array(":vid" => $vocabulary->vid, ":name" => $name));
      $FOUND = FALSE;
      foreach ($qresult as $key=>$record) {
        if($record->tid) {
          $term->tid = $record->tid;
          $FOUND = TRUE;
          break;
        }
      }

      $result = taxonomy_term_save($term);
      if ($result == 1) { // result == 2 is updating an existing term.
        print "-- $result: NEW TERM '$term->name' saved under ID: $term->tid.<br />\n";
      }

      return $term;
    }

  /**
   * Look for duplicates, based on the auxid. 
   *
   */
    function dosfixer_nodeaudit_finddups($node) {
      // Must have auxid field value.
      if(!empty($node->field_auxid['und'][0]['value'])) {
        $auxid = $node->field_auxid['und'][0]['value'];
        // See if another node with the same auxid and bundle/node-type exists. 
        $results = db_query(
          "SELECT aid.entity_id as nid " . 
          "FROM field_data_field_auxid aid " . 
          "WHERE aid.bundle = :bundle " . 
          "AND aid.field_auxid_value = :auxid " . 
          "AND aid.entity_id != :nid;"
          ,array(":bundle" => $node->type, ":auxid" => $auxid, ":nid" => $node->nid)
        );
        $nid = FALSE;
        foreach($results as $key=>$result) {
print "--$key<br />\n";
          $nid = $result->nid;
        }
print_r($nid);

        if($nid) {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: Duplicate found for auxid " . $auxid . ". See <a href=\"/node/$nid/edit\">$nid</a><br />\n";
          return TRUE;
        } else {
//print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: No duplicate found for auxid " . $auxid . ".<br />\n";
        }
      } else {
print "NODE: <a href=\"/node/$node->nid/edit\">$node->nid</a>: No auxid found.<br />\n";
      }
      
      return FALSE;
    }
