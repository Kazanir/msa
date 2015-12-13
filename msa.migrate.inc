<?php

function msa_migrate_api() {
  $schema = drupal_get_schema_unprocessed('msa');

  $api = array(
    'api' => 2,
    'groups' => array(
      'msa' => array(
        'title' => t('Import MSA/CSA data'),
      ),
    ),
    'migrations' => array(
      'MSA' => array(
        'class_name' => 'MsaMigration',
        'group_name' => 'msa',
        'file_var' => 'msa_migrate_county_file',
        'csv_cols' => array(
          0 => array('cbsa', 'CBSA Code'),
          2 => array('csa', 'CSA Code'),
          3 => array('title', 'CBSA Title'),
          4 => array('type', 'Metropolitan/Micropolitan Type'),
          9 => array('fips_state', 'State FIPS Code'),
          10 => array('fips_county', 'County FIPS Code'),
        ),
        'dest_table' => 'msa',
        'source_keys' => array(
          'fips_state' => $schema['msa_county']['fields']['fips_state'],
          'fips_county' => $schema['msa_county']['fields']['fips_county'],
        ),
      ),
      'MSACity' => array(
        'class_name' => 'MsaCityMigration',
        'group_name' => 'msa',
        'file_var' => 'msa_migrate_city_file',
        'csv_cols' => array(
          0 => array('cbsa', 'CBSA Code'),
          3 => array('city', 'City Name'),
          4 => array('fips_state', 'State FIPS Code'),
          5 => array('fips_place', 'City FIPS Code'),
        ),
        'dest_table' => 'msa_city',
        'source_keys' => array(
          'fips_state' => $schema['msa_city']['fields']['fips_state'],
          'fips_place' => $schema['msa_city']['fields']['fips_place'],
        ),
      ),
      'MSACounty' => array(
        'class_name' => 'MsaCountyMigration',
        'group_name' => 'msa',
        'file_var' => 'msa_migrate_county_file',
        'csv_cols' => array(
          0 => array('cbsa', 'CBSA Code'),
          1 => array('division_code', 'Metro Division Code'),
          5 => array('division', 'Metro Division'),
          7 => array('county', 'County Name'),
          8 => array('state_name', 'State Name'),
          9 => array('fips_state', 'State FIPS Code'),
          10 => array('fips_county', 'County FIPS Code'),
          11 => array('type', 'Central/Outlying Type'),
        ),
        'dest_table' => 'msa_county',
        'source_keys' => array(
          'fips_state' => $schema['msa_county']['fields']['fips_state'],
          'fips_county' => $schema['msa_county']['fields']['fips_county'],
        ),
      ),
      'MSAZip' => array(
        'class_name' => 'MsaZipMigration',
        'group_name' => 'msa',
        'file_var' => 'msa_migrate_zip_file',
        'csv_cols' => array(
          0 => array('zip', 'Zip Code'),
          1 => array('state', 'State Postal Abbreviation'),
          2 => array('cbsa', 'CBSA Code'),
        ),
        'dest_table' => 'msa_zip',
        'source_keys' => array(
          'zip' => $schema['msa_zip']['fields']['zip'],
        ),
      ),
      'CSA' => array(
        'class_name' => 'MsaCsaMigration',
        'group_name' => 'msa',
        'file_var' => 'msa_migrate_county_file',
        'csv_cols' => array(
          2 => array('csa', 'CSA Code'),
          6 => array('title', 'CSA Title'),
          9 => array('fips_state', 'State FIPS Code'),
          10 => array('fips_county', 'County FIPS Code'),
        ),
        'dest_table' => 'msa_csa',
        'source_keys' => array(
          'fips_state' => $schema['msa_county']['fields']['fips_state'],
          'fips_county' => $schema['msa_county']['fields']['fips_county'],
        ),
      ),
    ),
  );

  return $api;
}

class MsaBaseMigration extends Migration {
  public function __construct($arguments) {
    parent::__construct($arguments);

    $this->source = new MigrateSourceCSV(variable_get($arguments['file_var']), $arguments['csv_cols'], array('header_rows' => 1));
    $this->destination = new MigrateDestinationTable($arguments['dest_table']);

    $this->map = new MigrateSQLMap($this->machineName, $arguments['source_keys'], MigrateDestinationTable::getKeySchema($arguments['dest_table']));

    $schema = drupal_get_schema_unprocessed('msa');
    $fields = array_keys($schema[$arguments['dest_table']]['fields']);
    // Map everything in the schema directly.
    foreach ($fields as $field) {
      $this->addFieldMapping($field, $field);
    }
  }
}

class MsaMigration extends MsaBaseMigration {

  public function prepareRow($row) {
    $ret = parent::prepareRow($row);
    if ($ret === FALSE || empty($row->cbsa)) {
      return FALSE;
    }
    $row->type = strtolower(substr($row->type, 0, 5));

    // We might have multiple rows for a single MSA. Exclude all but the first.
    $map_row = db_query("SELECT * FROM migrate_map_msa WHERE destid1 = :cbsa;", array(':cbsa' => $row->cbsa))->fetch();
    if (!$map_row) {
      return TRUE;
    }
    elseif (($map_row->sourceid1 != $row->fips_state) || ($map_row->sourceid2 != $row->fips_place)) {
      return FALSE;
    }
  }

}

class MsaCityMigration extends MsaBaseMigration {

  public function prepareRow($row) {
    $ret = parent::prepareRow($row);
    if ($ret === FALSE) {
      return FALSE;
    }
    $row->state = msa_fips_state_retrieve($row->fips_state);
  }

}

class MsaCountyMigration extends MsaBaseMigration {

  public function prepareRow($row) {
    $ret = parent::prepareRow($row);
    if ($ret === FALSE) {
      return FALSE;
    }
    $row->state = msa_fips_state_retrieve($row->fips_state);
    $row->type = strtolower($row->type);
  }
}

class MsaCsaMigration extends MsaBaseMigration {

  public function prepareRow($row) {
    $ret = parent::prepareRow($row);
    if ($ret === FALSE || empty($row->csa) || empty($row->fips_state)) {
      return FALSE;
    }

    // We might have multiple rows for a single CSA. Exclude all but the first.
    $map_row = db_query("SELECT * FROM migrate_map_csa WHERE destid1 = :csa;", array(':csa' => $row->csa))->fetch();
    if (!$map_row) {
      return TRUE;
    }
    elseif (($map_row->sourceid1 != $row->fips_state) || ($map_row->sourceid2 != $row->fips_place)) {
      return FALSE;
    }
  }

}

class MsaZipMigration extends MsaBaseMigration {

  public function prepareRow($row) {
    $ret = parent::prepareRow($row);
    if ($ret === FALSE || empty($row->cbsa)) {
      return FALSE;
    }
  }

}

