/* Copyright (c) 2018-2020, Bruno de Lacheisserie
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - Neither the name of nor the names of its contributors may be used to
 *   endorse or promote products derived from this software without specific
 *   prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

package org.LK8000;

import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.CancellationSignal;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;
import android.provider.DocumentsContract.Root;
import android.provider.DocumentsProvider;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Collections;
import java.util.LinkedList;
import java.util.Objects;

public class AllFilesDocumentsProvider extends DocumentsProvider {
    private final static String AUTHORITY = BuildConfig.APPLICATION_ID + ".allfiles";

    private static final String[] DEFAULT_ROOT_PROJECTION = {
            Root.COLUMN_ROOT_ID,
            Root.COLUMN_TITLE,
            Root.COLUMN_ICON,
            Root.COLUMN_FLAGS,
            Root.COLUMN_DOCUMENT_ID
    };

    private static final String[] DEFAULT_DOCUMENT_PROJECTION = {
            Document.COLUMN_DOCUMENT_ID,
            Document.COLUMN_DISPLAY_NAME,
            Document.COLUMN_MIME_TYPE,
            Document.COLUMN_SIZE,
            Document.COLUMN_LAST_MODIFIED,
//            COLUMN_ICON
//            COLUMN_SUMMARY
            Document.COLUMN_FLAGS
    };

    private static final int MAX_SEARCH_RESULTS = 20;

    /**
     * @param projection the requested root column projection
     * @return either the requested root column projection, or the default projection if the
     * requested projection is null.
     */
    private static String[] resolveRootProjection(String[] projection) {
        return projection != null ? projection : DEFAULT_ROOT_PROJECTION;
    }

    private static String[] resolveDocumentProjection(String[] projection) {
        return projection != null ? projection : DEFAULT_DOCUMENT_PROJECTION;
    }

    private ContentResolver getContentResolver() {
        return getContext().getContentResolver();
    }

    private static Uri getChildDocumentsUri(String parentDocumentId) {
        return DocumentsContract.buildChildDocumentsUri(AUTHORITY, parentDocumentId);
    }

    private void notifyChildDocumentsChange(String parentDocumentId) {
        getContentResolver().notifyChange(getChildDocumentsUri(parentDocumentId),null, false);
    }

    private File getRootDir() {
        Context context = Objects.requireNonNull(getContext());
        return context.getExternalFilesDir(null);
    }

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public Cursor queryRoots(String[] projection) throws FileNotFoundException {
        MatrixCursor result = new MatrixCursor(resolveRootProjection(projection));
        MatrixCursor.RowBuilder row = result.newRow();

        Context context = Objects.requireNonNull(getContext());

        File file = getRootDir();

        row.add(Root.COLUMN_ROOT_ID, "RootId");
        row.add(Root.COLUMN_DOCUMENT_ID, getDocIdForFile(file));

        ApplicationInfo applicationInfo = context.getApplicationInfo();

        row.add(Root.COLUMN_ICON, applicationInfo.icon);
        row.add(Root.COLUMN_TITLE, context.getString(applicationInfo.labelRes));

        row.add(Root.COLUMN_FLAGS, Root.FLAG_LOCAL_ONLY
                | Root.FLAG_SUPPORTS_SEARCH
                | Root.FLAG_SUPPORTS_CREATE);

        result.setNotificationUri(getContentResolver(), getChildDocumentsUri("RootId"));
        return (result);
    }

    @Override
    public Cursor queryChildDocuments(String parentDocumentId, String[] projection, String sortOrder) throws FileNotFoundException {
        MatrixCursor result = new MatrixCursor(resolveDocumentProjection(projection));
        try {
            File filesDir = getFileForDocId(parentDocumentId);
            for (File file : Objects.requireNonNull(filesDir.listFiles())) {
                // Don't show hidden files/folders
                if (!file.getName().startsWith(".")) {
                    addDocumentRow(result, file);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
            throw new FileNotFoundException();
        }
        result.setNotificationUri(getContentResolver(), getChildDocumentsUri(parentDocumentId));
        return (result);
    }

    @Override
    public Cursor queryDocument(String documentId, String[] projection) throws FileNotFoundException {
        MatrixCursor result = new MatrixCursor(resolveDocumentProjection(projection));
        try {
            addDocumentRow(result, getFileForDocId(documentId));
        } catch (Exception e) {
            e.printStackTrace();
            throw new FileNotFoundException();
        }
        result.setNotificationUri(getContentResolver(), getChildDocumentsUri(documentId));
        return (result);
    }

    @Override
    public String getDocumentType(final String documentId) throws FileNotFoundException {
        return FileUtils.getDocumentType(getFileForDocId(documentId));
    }

    private String getDocIdForFile(@NonNull File file) {
        File rootFile = getRootDir();
        return "root:" + rootFile.toURI().relativize(file.toURI()).toString();
    }

    private File getFileForDocId(String documentId) throws FileNotFoundException {
        String decoded = Uri.decode(documentId);
        final int splitIndex = decoded.indexOf(':', 1);
        final String path = decoded.substring(splitIndex + 1);
        final File rootFile = getRootDir();
        File file = new File(rootFile, path);
        if (file.exists()) {
            return file;
        }
        throw new FileNotFoundException();
    }

    private void addDocumentRow(MatrixCursor result, File file) throws FileNotFoundException {
        MatrixCursor.RowBuilder row = result.newRow();

        row.add(Document.COLUMN_DOCUMENT_ID, getDocIdForFile(file));
        row.add(Document.COLUMN_DISPLAY_NAME, file.getName());
        row.add(Document.COLUMN_SIZE, file.length());
        row.add(Document.COLUMN_LAST_MODIFIED, file.lastModified());
        row.add(Document.COLUMN_MIME_TYPE, FileUtils.getDocumentType(file));

        int flags = 0;
        if (file.canWrite()) {
            if (file.isDirectory()) {
                // allow to add file in directory
                flags |= Document.FLAG_DIR_SUPPORTS_CREATE;
            } else {
                // allow to modify existing file
                flags |= Document.FLAG_SUPPORTS_WRITE;
            }

            // allow to delete Existing file or directory
            flags |= Document.FLAG_SUPPORTS_DELETE;
            // allow to rename existing file or directory
            flags |= Document.FLAG_SUPPORTS_RENAME;
        }
        row.add(Document.COLUMN_FLAGS, flags);
    }

    @Override
    public ParcelFileDescriptor openDocument(String documentId, String mode, @Nullable CancellationSignal signal) throws FileNotFoundException {
        return ParcelFileDescriptor.open(getFileForDocId(documentId), ParcelFileDescriptor.parseMode(mode));
    }

    @Override
    public void deleteDocument(final String documentId) throws FileNotFoundException {
        File file = getFileForDocId(documentId);
        if (FileUtils.deleteDirectory(file)) {
            File parentFile = file.getParentFile();
            if (parentFile != null) {
                String parentDocumentId = getDocIdForFile(file.getParentFile());
                notifyChildDocumentsChange(parentDocumentId);
            }
        } else {
            throw new FileNotFoundException("Failed to delete document id : " + documentId);
        }
    }

    @Override
    public Cursor querySearchDocuments(String rootId, String query, String[] projection) throws FileNotFoundException {
        final MatrixCursor result = new MatrixCursor(resolveDocumentProjection(projection));
        final File parent = getRootDir();
        final LinkedList<File> pending = new LinkedList<>();
        final String lowerQuery = query.toLowerCase();

        // Start by adding the parent to the list of files to be processed
        pending.add(parent);

        // Do while we still have unexamined files, and fewer than the max search results
        while (!pending.isEmpty() && result.getCount() < MAX_SEARCH_RESULTS) {
            // Take a file from the list of unprocessed files
            final File file = pending.removeFirst();
            if (file.isDirectory()) {
                // If it's a directory, add all its children to the unprocessed list
                Collections.addAll(pending, file.listFiles());
            } else {
                // If it's a file and it matches, add it to the result cursor.
                if (file.getName().toLowerCase().contains(lowerQuery)) {
                    addDocumentRow(result, file);
                }
            }
        }
        return result;
    }

    @Override
    public String createDocument(String parentDocumentId, String mimeType, String displayName) throws FileNotFoundException {
        final File parent = getFileForDocId(parentDocumentId);
        if (!parent.isDirectory()) {
            throw new IllegalArgumentException("Parent document isn't a directory");
        }
        File file = new File(parent, displayName);

        if (Document.MIME_TYPE_DIR.equals(mimeType)) {
            if (!file.mkdirs()) {
                throw new FileNotFoundException("Failed to create directory with name " +
                        displayName + " and documentId " + parentDocumentId);
            }
        } else {
            try {
                file.createNewFile();
                file.setWritable(true);
                file.setReadable(true);
            } catch (IOException e) {
                throw new FileNotFoundException("Failed to create document with name " +
                        displayName + " and documentId " + parentDocumentId);
            }
        }

        notifyChildDocumentsChange(parentDocumentId);

        return getDocIdForFile(file);
    }

    @Override
    public String renameDocument(String documentId, String displayName) throws FileNotFoundException {
        File srcFile = getFileForDocId(documentId);
        File parentFile = srcFile.getParentFile();
        File dstFile = new File(parentFile, displayName);
        if (!srcFile.renameTo(dstFile)) {
            throw new FileNotFoundException("Failed to rename document with name " +
                    displayName + " and documentId " + documentId);
        }
        if (parentFile != null) {
            notifyChildDocumentsChange(getDocIdForFile(parentFile));
        } else {
            notifyChildDocumentsChange("RootId");
        }
        return getDocIdForFile(dstFile);
    }

}
