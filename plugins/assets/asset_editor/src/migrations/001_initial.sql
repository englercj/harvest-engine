-- Copyright Chad Engler

CREATE TABLE asset_file (
    id                      INTEGER PRIMARY KEY,    -- Primary key for a row.
    uuid                    BLOB(16) NOT NULL,      -- Globally unique ID of the Asset File.
    file_path               TEXT NOT NULL,          -- On-disk path to the *.assets file, relative to the project root.
    file_write_time         INTEGER DEFAULT 0,      -- Last "write time" we read from the file on disk.
    file_size               INTEGER DEFAULT 0,      -- Last size of the file we read from disk.
    source_path             TEXT DEFAULT NULL,      -- On-disk path to the source data for assets, relative to the asset file.
    source_write_time       INTEGER DEFAULT 0,      -- Last "write time" we read from the import source on disk.
    source_size             INTEGER DEFAULT 0,      -- Last size of the file we read from the import source on disk.
    scan_token              INTEGER DEFAULT 0,      -- An identifier from the last file scanner that checked this entry.
    UNIQUE (id),
    UNIQUE (file_path)
) WITHOUT ROWID;
CREATE INDEX idx_asset_file_source_path ON asset_file (source_path);

CREATE TABLE asset (
    id                      INTEGER PRIMARY KEY,    -- Primary key for a row.
    uuid                    BLOB(16) NOT NULL,      -- Globally unique ID of the Asset.
    asset_file_id           INTEGER NOT NULL,       -- The file this asset exists in.
    asset_type_name         TEXT NOT NULL,          -- Type of the Asset.
    name                    TEXT NOT NULL,          -- Friendly name of the Asset.
    state                   INTEGER NOT NULL,       -- `he::assets::AssetState` enum value.
    data_hash               INTEGER DEFAULT 0,      -- Last recorded Fnv32 hash of the asset's data bytes.
    import_data_hash        INTEGER DEFAULT 0,      -- Last recorded Fnv32 hash of the asset's import data bytes.
    importer_id             INTEGER DEFAULT 0,      -- ID of the last importer to process this asset.
    importer_version        INTEGER DEFAULT 0,      -- Version of the importer that processed this asset.
    compiler_id             INTEGER DEFAULT 0,      -- ID of the last compiler to process this asset.
    compiler_version        INTEGER DEFAULT 0,      -- Version of the compiler that processed this asset.
    UNIQUE (uuid),
    FOREIGN KEY (asset_file_id) REFERENCES asset_file (id) ON DELETE CASCADE
) WITHOUT ROWID;
CREATE INDEX idx_asset_fk_asset_file_id ON asset (asset_file_id);
CREATE INDEX idx_asset_state ON asset (state);

CREATE TABLE config (
    id                      INTEGER PRIMARY KEY,    -- Primary key for a row.
    key                     TEXT NOT NULL,          -- The key for the config entry.
    value                   BLOB DEFAULT NULL,      -- The value of the config entry.
    UNIQUE (key)
) WITHOUT ROWID;

CREATE TABLE tag (
    id                      INTEGER PRIMARY KEY,    -- Primary key for a row.
    name                    TEXT NOT NULL,          -- The name of this tag entry.
    UNIQUE (name)
) WITHOUT ROWID;

CREATE TABLE asset_tag (
    id                      INTEGER PRIMARY KEY,    -- Primary key for a row.
    asset_id                INTEGER NOT NULL,       -- The id of the asset the tag is attached to.
    tag_id                  INTEGER NOT NULL,       -- The id of the tag attached to the asset.
    FOREIGN KEY (asset_id) REFERENCES asset (id) ON DELETE CASCADE,
    FOREIGN KEY (tag_id) REFERENCES tag (id) ON DELETE CASCADE
) WITHOUT ROWID;
CREATE INDEX idx_asset_tag_fk_asset_id ON asset_tag (asset_id);
CREATE INDEX idx_asset_tag_fk_tag_rowid ON asset_tag (tag_id);

CREATE TABLE asset_reference (
    id                      INTEGER PRIMARY KEY,    -- Primary key for a row.
    from_id                 INTEGER NOT NULL,       -- Asset that the reference is from.
    to_id                   INTEGER NOT NULL,       -- Asset that the reference is to.
    FOREIGN KEY (from_id) REFERENCES asset (id) ON DELETE CASCADE,
    FOREIGN KEY (to_id) REFERENCES asset (id) ON DELETE CASCADE
) WITHOUT ROWID;
CREATE INDEX idx_asset_reference_fk_from_id ON asset_reference (from_id);
CREATE INDEX idx_asset_reference_fk_to_id ON asset_reference (to_id);

CREATE TABLE compiler_reference (
    id                      INTEGER PRIMARY KEY,    -- Primary key for a row.
    asset_id                INTEGER NOT NULL,       -- The Asset that was compiling when we discovered this reference.
    kind                    INTEGER NOT NULL,       -- `CompilerReferenceModel::Kind` enum value.
    to_asset_id             INTEGER,                -- The asset who's data this compile requested.
    file_path               TEXT,                   -- The path to the file that was loaded by a compile.
    resource_key_type       INTEGER,                -- Type of the resource key for the resource that was requested.
    resource_key_hash       INTEGER,                -- The hash of the resource key that was requested.
    FOREIGN KEY (asset_id) REFERENCES asset (id) ON DELETE CASCADE
) WITHOUT ROWID;
CREATE INDEX idx_compiler_reference_fk_asset_id ON compiler_reference (asset_id);

CREATE TABLE message (
    id                      INTEGER PRIMARY KEY,    -- Primary key for a row.
    ref_id                  INTEGER NOT NULL,       -- Asset or Asset File this message is for.
    level                   INTEGER NOT NULL,       -- Log level of the message.
    message                 TEXT NOT NULL,          -- The text of the message.
    timestamp               INTEGER NOT NULL,       -- Nanosecond timestamp of the message.
    source                  INTEGER NOT NULL        -- `MessageMode::Source` enum value.
) WITHOUT ROWID;
CREATE INDEX idx_message_ref_id ON message (ref_id);

-- Full-Text searching of the asset table.
CREATE VIRTUAL TABLE fts_asset USING fts5(name, content='asset', content_rowid='id');

CREATE TRIGGER asset_ai AFTER INSERT ON asset BEGIN
    INSERT INTO fts_asset(rowid, name) VALUES (new.id, new.name);
END;

CREATE TRIGGER asset_ad AFTER DELETE ON asset BEGIN
    INSERT INTO fts_asset(fts_asset, rowid, name) VALUES ('delete', old.id, old.name);
END;

CREATE TRIGGER asset_au AFTER UPDATE ON asset BEGIN
    INSERT INTO fts_asset(fts_asset, rowid, name) VALUES ('delete', old.id, old.name);
    INSERT INTO fts_asset(rowid, name) VALUES (new.id, new.name);
END;

-- Full-Text searching of the tag table.
CREATE VIRTUAL TABLE fts_tag USING fts5(name, content='tag', content_rowid='id');

CREATE TRIGGER tag_ai AFTER INSERT ON tag BEGIN
    INSERT INTO fts_tag(rowid, name) VALUES (new.id, new.name);
END;

CREATE TRIGGER tag_ad AFTER DELETE ON tag BEGIN
    INSERT INTO fts_tag(fts_tag, rowid, name) VALUES ('delete', old.id, old.name);
END;

CREATE TRIGGER tag_au AFTER UPDATE ON tag BEGIN
    INSERT INTO fts_tag(fts_tag, rowid, name) VALUES ('delete', old.id, old.name);
    INSERT INTO fts_tag(rowid, name) VALUES (new.id, new.name);
END;
