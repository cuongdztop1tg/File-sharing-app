| **Bước**           | **Lệnh (Gõ trên Client)**                                                             | **Kết quả mong đợi**                                                                                                                              |
| -------------------------- | ---------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **1. Đăng nhập**  | `LOGIN user1 123`                                                                            | Báo `Login successful`.                                                                                                                                  |
| **2. Xem file**      | `LIST`                                                                                       | Server trả về danh sách trống (nếu chưa có file nào) hoặc danh sách cũ.                                                                          |
| **3. Upload nhỏ**   | `UPLOAD test_upload.txt`                                                                     | Client báo `File sent`. Server (bên kia) báo `Upload completed`. Kiểm tra folder `data/files/`xem file có ở đó không.                        |
| **4. Xem lại**      | `LIST`                                                                                       | Phải hiện tên `test_upload.txt`.                                                                                                                       |
| **5. Test Download** | (Xóa file gốc ở client trước)`rm test_upload.txt``DOWNLOAD test_upload.txt` | Client báo `Download completed`. File `test_upload.txt`xuất hiện lại ở thư mục client.                                                           |
| **6. Xóa file**     | `DELETE test_upload.txt`                                                                     | Server báo `File deleted`. Dùng `LIST`để kiểm tra file đã mất chưa.                                                                            |
| **7. Upload lớn**   | `UPLOAD test_large.bin`                                                                      | Quan sát thành log. Bạn sẽ thấy dòng `sending...`chạy liên tục hoặc thanh tiến trình giả lập (nếu có). Server phải nhận đủ số bytes. |




#### Các lệnh cần nhập trong Client để test:

1. **Đăng nhập:**
   **Plaintext**

   ```
   LOGIN admin 123456
   ```
2. **Upload file mẫu:**
   **Plaintext**

   ```
   UPLOAD test.txt
   ```
3. **Tạo thư mục (MKDIR):**
   **Plaintext**

   ```
   MKDIR tailieu
   LIST
   ```

   *(Kiểm tra xem có thấy thư mục `tailieu` không)*
4. **Copy file (COPY):**
   **Plaintext**

   ```
   COPY test.txt ban_sao.txt
   LIST
   ```

   *(Kiểm tra xem có thêm `ban_sao.txt` không)*
5. **Đổi tên (RENAME):**
   **Plaintext**

   ```
   RENAME ban_sao.txt file_moi.txt
   LIST
   ```
6. **Di chuyển vào thư mục (MOVE):**
   **Plaintext**

   ```
   MOVE file_moi.txt tailieu/file_moi.txt
   LIST
   ```

   *(File sẽ biến mất khỏi danh sách ngoài)*
7. **Xóa thư mục (DELETE) - Phần quan trọng nhất:**
   **Plaintext**

   ```
   DELETE tailieu
   LIST
   ```

   *(Nếu code đệ quy đúng, thư mục `tailieu` sẽ bị xóa dù bên trong có file)*

Nếu tất cả các bước trên đều báo `[SUCCESS]`, chúc mừng bạn đã hoàn thành xuất sắc Module 3!
