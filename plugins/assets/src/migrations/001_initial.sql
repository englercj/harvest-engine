-- Copyright Chad Engler

CREATE TABLE asset_file (
    id                      BLOB(16) NOT NULL,      -- Globally unique ID of the Asset File.
    path                    TEXT NOT NULL,          -- On-disk path to the *.assets file, relative to the project root.
    last_modified_time      INTEGER DEFAULT 0,      -- Last "modified time" we read from the file on disk.
    last_file_size          INTEGER DEFAULT 0,      -- Last size of the file we read from disk.
    last_session_token      INTEGER DEFAULT 0,      -- An identifier that informs if this entry is up-to-date with the latest launch.
    source_modified_time    INTEGER DEFAULT 0,      -- Last "modified time" we read from the import source on disk.
    source_file_size        INTEGER DEFAULT 0,      -- Last size of the file we read from the import source on disk.
    PRIMARY KEY (id),
    UNIQUE (path)
);

CREATE TABLE asset_source (
    id                      BLOB(16) NOT NULL,      -- Globally unique ID of the Asset File.
    path                    TEXT NOT NULL,          -- On-disk path to the *.assets file, relative to the project root.
    last_modified_time      INTEGER DEFAULT 0,      -- Last "modified time" we read from the file on disk.
    last_file_size          INTEGER DEFAULT 0,      -- Last size of the file we read from disk.
    last_session_token      INTEGER DEFAULT 0,      -- An identifier that informs if this entry is up-to-date with the latest launch.
    source_modified_time    INTEGER DEFAULT 0,      -- Last "modified time" we read from the import source on disk.
    source_file_size        INTEGER DEFAULT 0,      -- Last size of the file we read from the import source on disk.
    PRIMARY KEY (id),
    UNIQUE (path)
);

CREATE TABLE asset (
    id                      BLOB(16) NOT NULL,      -- Globally unique ID of the Asset.
    asset_file_id           BLOB(16) NOT NULL,      -- The file this asset exists in.
    type                    TEXT NOT NULL,          -- Type of the Asset.
    name                    TEXT NOT NULL,          -- Friendly name of the Asset.
    state                   INTEGER NOT NULL,       -- `AssetState` enum value.
    data_hash               INTEGER DEFAULT 0,      -- Last recorded Fnv32 hash of the asset's data bytes.
    import_data_hash        INTEGER DEFAULT 0,      -- Last recorded Fnv32 hash of the asset's import data bytes.
    importer_id             INTEGER DEFAULT 0,      -- ID of the last importer to process this asset.
    importer_version        INTEGER DEFAULT 0,      -- Version of the importer that processed this asset.
    compiler_id             INTEGER DEFAULT 0,      -- ID of the last compiler to process this asset.
    compiler_version        INTEGER DEFAULT 0,      -- Version of the compiler that processed this asset.
    PRIMARY KEY (id),
    FOREIGN KEY (asset_file_id) REFERENCES asset_file (id) ON DELETE CASCADE
);
CREATE INDEX idx_asset_fk_asset_file_id ON asset (asset_file_id);
CREATE INDEX idx_asset_state ON asset (state);

CREATE TABLE asset_backing_file (
    asset_id                BLOB(16) NOT NULL,      -- The Asset ID this backing file belongs to.
    path                    TEXT NOT NULL,          -- On-disk path to the backing file, relative to the asset file.
    last_modified_time      INTEGER,                -- Last "modified time" we read from the file on disk.
    last_file_size          INTEGER,                -- Last size of the file we saw when interacting with this file.
    UNIQUE (path),
    FOREIGN KEY (asset_id) REFERENCES asset (id) ON DELETE CASCADE
);
CREATE INDEX idx_asset_backing_file_fk_asset_id ON asset_backing_file (asset_id);

CREATE TABLE tag (
    tag                     TEXT NOT NULL,          -- The text of this tag
    UNIQUE (tag)
);

CREATE TABLE asset_tag (
    asset_id                BLOB(16) NOT NULL,      -- Asset id this tag maps to.
    tag_rowid               INTEGER NOT NULL,       -- The rowid of the tag attached to the asset.
    FOREIGN KEY (asset_id) REFERENCES asset (id) ON DELETE CASCADE,
    FOREIGN KEY (tag_rowid) REFERENCES tag (rowid) ON DELETE CASCADE
);
CREATE INDEX idx_asset_tag_fk_asset_id ON asset_tag (asset_id);
CREATE INDEX idx_asset_tag_fk_tag_rowid ON asset_tag (tag_rowid);

CREATE TABLE asset_reference (
    from_id                 BLOB(16) NOT NULL,      -- Asset that the reference is from.
    to_id                   BLOB(16) NOT NULL,      -- Asset that the reference is to.
    FOREIGN KEY (from_id) REFERENCES asset (id) ON DELETE CASCADE,
    FOREIGN KEY (to_id) REFERENCES asset (id) ON DELETE CASCADE
);
CREATE INDEX idx_asset_reference_fk_from_id ON asset_reference (from_id);
CREATE INDEX idx_asset_reference_fk_to_id ON asset_reference (to_id);

CREATE TABLE message (
    ref_id                  BLOB(16) NOT NULL,      -- Asset or Asset File this message is for.
    level                   INTEGER NOT NULL,       -- Log level of the message.
    message                 TEXT NOT NULL,          -- The text of the message.
    timestamp               INTEGER NOT NULL,       -- Nanosecond timestamp of the message.
    source                  INTEGER NOT NULL        -- `MessageSource` enum value the source of the message.
);
CREATE INDEX idx_message_fk_ref_id ON message (ref_id);

CREATE TABLE compiler_reference (
    asset_id                BLOB(16) NOT NULL,      -- The Asset that was compiling when we discovered this reference.
    reference_type          INTEGER NOT NULL,       -- `CompilerReferenceType` enum value.
    to_asset_id             BLOB(16),               -- The asset who's data this compile requested.
    file_path               TEXT,                   -- The path to the file that was loaded by a compile.
    resource_key_type       INTEGER,                -- Type of the resource key for the resource that was requested.
    resource_key_hash       INTEGER,                -- The hash of the resource key that was requested.
    FOREIGN KEY (asset_id) REFERENCES asset (id) ON DELETE CASCADE
);
CREATE INDEX idx_compiler_reference_asset_id ON compiler_reference (asset_id);

-- Full-Text searching of the asset table.
CREATE VIRTUAL TABLE fts_asset USING fts5(id, name, content='asset');

CREATE TRIGGER asset_ai AFTER INSERT ON asset BEGIN
    INSERT INTO fts_asset(rowid, id, name) VALUES (new.rowid, new.id, new.name);
END;

CREATE TRIGGER asset_ad AFTER DELETE ON asset BEGIN
    INSERT INTO fts_asset(fts_asset, rowid, id, name) VALUES ('delete', old.rowid, old.id, old.name);
END;

CREATE TRIGGER asset_au AFTER UPDATE ON asset BEGIN
    INSERT INTO fts_asset(fts_asset, rowid, id, name) VALUES ('delete', old.rowid, old.id, old.name);
    INSERT INTO fts_asset(rowid, id, name) VALUES (new.rowid, new.id, new.name);
END;
