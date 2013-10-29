<?php

/**
 * @file
 * Defines RefworksXMLImporter.
 */

class RefworksXMLImporter extends ScholarBatchImporter {
  protected $itemClass = 'RefworksXMLImportObject';

  /**
   * Get the number of items to import from $this->file.
   *
   * @see IslandoraBatchImporter::getNumber()
   */
  public function getNumber() {
    $refworks = new DOMDocument();
    $refworks->load($this->file->uri);
    $xpath = new DOMXPath($refworks);
    $results = $xpath->query('/refworks/reference');
    return $results ? $results->length : 0;
  }

  /**
   * Get the form for this importer.
   *
   * @see IslandoraBatchImporter::getForm()
   */
  public static function getForm(array &$form_state) {
    return array(
      'fs' => array(
        '#type' => 'fieldset',
        '#title' => t('Refworks Batch Importer'),
        'file' => array(
          '#type' => 'managed_file',
          '#title' => t('File of Refworks XML records to import (".xml" extension).'),
          '#upload_validators' => array(
            'file_validate_extensions' => array('xml'),
          ),
        ),
        'submit' => array(
          '#type' => 'submit',
          '#value' => t('Import'),
        ),
      ),
    );
  }

  /**
   * Determine if we're ready to run our batch process.
   *
   * @see IslandoraBatchImporter::readyForBatch()
   */
  public static function readyForBatch(array &$form_state) {
    return !empty($form_state['values']['file']);
  }

  /**
   * Get the required info to pass into the file parameter of the constructor.
   *
   * @see IslandoraBatchImporter::getBatchInfo()
   */
  public static function getBatchInfo(array &$form_state) {
    $file = file_load($form_state['values']['file']);
    return $file;
  }
}

/**
 * RIS import object.
 *
 * Actually does the heavy-lifting during the import.
 * @author adam
 */
class RefworksXMLImportObject extends IslandoraImportObject {
  protected $mods;

  /**
   * Get an item from the source.
   *
   * @see IslandoraImportObject::getOne()
   */
  public static function getOne(&$file) {
    $record = '';

    $refworks = new DOMDocument();
    $refworks->load($file->uri);
    $xpath = new DOMXPath($refworks);
    $results = $xpath->query('/refworks/reference');
    $documents = array();
    if ($results->length >= 1) {
      // Get Record.
      $child = $results->item(0);
      $record = '<refworks>' . $refworks->saveXML($child) . '</refworks>';
      // Remove Record.
      $child->parentNode->removeChild($child);
      $refworks->save($file->uri);
      file_save($file);
    }
    return (empty($record) ? FALSE : new self($record));
  }

  /**
   * Generates a MODS document repersenting the imported data.
   *
   * @see IslandoraImportObject::getMODS()
   */
  public function getMODS() {
    if ($this->mods === NULL) {
      $path = drupal_get_path('module', 'islandora_refworksxml_importer');
      $refworks = new DOMDocument();
      $refworks->loadXML($this->source);
      $genre = $refworks->getElementsByTagName('rt');
      $genre_string = $genre->item(0)->nodeValue;
      if (empty($genre)) {
        return FALSE;
      }
      switch ($genre_string) {
        case 'Book':
          $xsl_path = $path . '/xsl/refworks_to_mods_book.xsl';
          $genre->item(0)->nodeValue = 'book';
          break;

        case 'Book, Section':
        case 'Book, Chapter':
          $xsl_path = $path . '/xsl/refworks_to_mods_book_section.xsl';
          $genre->item(0)->nodeValue = 'chapter';
          break;

        case 'Conference Proceedings':
          $xsl_path = $path . '/xsl/refworks_to_mods_conf.xsl';
          $genre->item(0)->nodeValue = 'paper-conference';
          break;

        case 'Newspaper Article':
          $xsl_path = $path . '/xsl/refworks_to_mods_journal.xsl';
          $genre->item(0)->nodeValue = 'article-newspaper';
          break;

        default:
          $xsl_path = $path . '/xsl/refworks_to_mods_journal.xsl';
          $genre->item(0)->nodeValue = 'article-journal';
      }
      $xslt = new XSLTProcessor();
      $xsl_doc = new DomDocument();
      $xsl_doc->load($xsl_path);
      $xslt->importStylesheet($xsl_doc);
      $this->mods = $xslt->transformToXml($refworks);
    }
    return $this->mods;
  }
}
