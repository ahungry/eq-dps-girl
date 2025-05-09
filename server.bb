#! /usr/bin/env bb
#_" -*- mode: clojure; -*-"

;; C-c M-c

(ns my-server
  (:require [babashka.fs :as fs]
            [clojure.java.io :as io]
            [clojure.java.browse :as browse]
            [clojure.string :as str]
            [clojure.tools.cli :refer [parse-opts]]
            [org.httpkit.server :as server]
            [hiccup2.core :as html])
  (:import [java.net URLDecoder URLEncoder]
           [java.text SimpleDateFormat]
           [java.util Date TimeZone]
           [java.time.format DateTimeFormatter]
           [java.time LocalDateTime]
           [java.io RandomAccessFile]))

;; FIXME: You must update these for your own system for now!
(defonce *path* (str (System/getProperty "user.home") "/src/eq-dps-girl/"))
(defonce *logpath* (str (System/getProperty "user.home") "/games/wine/thj-take2/drive_c/thj/Logs/"))

;; NOTE: You can adjust these if you feel you're having performance issues
;; but the parsing should always tail the log file and not re-read, so even
;; massive files should be fine.
;; The inactivity is the timer until dps-girl falls asleep after you leave combat,
;; When she falls asleep, old combat data is purged and not part of the active meter.
(defonce *dps-inactivity-seconds* 3)
(defonce *parse-interval* 50)

;; NOTE: Do not touch these or anything below this line
(defonce server (atom nil))
(defonce *channel* (atom nil))
(defonce *stream* (atom false))

(defn parse-date-to-epoch [date-string]
  (try
    (let [format-string "EEE MMM dd HH:mm:ss yyyy"
          formatter (SimpleDateFormat. format-string)]
      (.setTimeZone formatter (TimeZone/getTimeZone (System/getProperty "user.timezone")))
      (let [date (.parse formatter date-string)]
        (/ (.getTime date) 1000)))
    (catch Exception e
      (println "Error parsing date string:" (.getMessage e))
      nil)))

(defn get-formatted-local-date []
  (let [now (LocalDateTime/now)
        formatter (DateTimeFormatter/ofPattern "E MMM dd HH:mm:ss yyyy")]
    (.format formatter now)))

(defn epoch-local []
  (parse-date-to-epoch (get-formatted-local-date)))

(defn get-logs-in-dir
  "Get the logs in directory that appear to be an eq log."
  [dir]
  (let [directory (clojure.java.io/file dir)
        files (file-seq directory)]
    (filter #(and (re-matches #".*eqlog_.*\.txt$" (.toString %))) files)))

(defn get-newest-log-in-dir
  "Sort File results by last modification date."
  []
  (->> (get-logs-in-dir *logpath*)
       (sort (fn [a b] (> (.lastModified a) (.lastModified b))))
       first))

(defn read-file-size [file-path]
  (with-open [raf (RandomAccessFile. file-path "r")]
    (.length raf)))

(defn read-from-pos [file-path pos]
  (with-open [raf (RandomAccessFile. file-path "r")]
    (.seek raf pos)
    (-> (loop [byte (.read raf)
               bytes []]
          (if (= byte -1)
            bytes
            (recur (.read raf) (conj bytes (char byte)))))
        clojure.string/join)))

(defonce *last-pos* (atom 0))
(defonce *stats* (atom []))

(defn slurp-once [file-path]
  (let [content (read-from-pos file-path @*last-pos*)]
    (swap! *last-pos* #(+ % (.length content)))
    content))

(defn reset []
  (reset! *last-pos* 0)
  (reset! *stats* []))

;; Currently reads the entire file - we probably want to break this
;; into some way to read from last index of file
(defn get-hits []
  (->> (clojure.string/split (slurp-once (get-newest-log-in-dir)) #"\r\n")
       (map #(re-find #".* points of.*damage" %))
       (filter some?)
       (filter #(not (re-find #"(\] a|healed|YOU)" %)))
       (map #(re-find #"\[(.*?)\].*? (\d+)" %))
       (map (fn [[_ date damage]] {:epoch (parse-date-to-epoch date)
                                   :damage (Integer/parseInt damage)}))))

(defn get-inactivity-seconds []
  (if (> (count @*stats*) 0)
    (- (epoch-local) (:epoch (last @*stats*)))
    -1))

(defn update-stats []
  (swap! *stats* into (get-hits))
  ;; If nothing has happened combat wise in 30 seconds, reset the active stats
  (when (> (get-inactivity-seconds) *dps-inactivity-seconds*)
    (reset! *stats* [])))

(defn get-duration-seconds []
  (max 1 (if (> (count @*stats*) 0)
           (- (:epoch (last @*stats*))
              (:epoch (first @*stats*)))
           -1)))

(defn get-sum-damage []
  (if (> (count @*stats*) 0)
    (reduce #(+ %1 (:damage %2)) 0 @*stats*)
    -1))

(defn get-dps []
  (int (float (/ (get-sum-damage) (get-duration-seconds)))))

(defn get-text [name]
  (slurp (str *path* name)))

(defn get-file [name]
  (fs/file (str *path* name)))

(defn send-msg [s]
  (if @*channel*
    (server/send! @*channel* (str "data: " s "\n\n") false)
    (prn "Missing a connected channel/client to send to!")))

(defn stop-stream []
  (reset! *stream* false))

(defn start-stream []
  (reset! *stream* true)
  (future
    (while @*stream*
      (Thread/sleep *parse-interval*)
      (update-stats)
      (send-msg (str (get-dps))))))

(defn route [req]
  (case (:uri req)
    "/" {:status 200
         :headers {"Content-Type" "text/html"}
         :body
         (get-text "main.html")}

    "/stream" (server/with-channel req channel
                (server/send!
                 channel
                 {:status 200
                  :headers {"Content-Type" "text/event-stream" "Cache-Control" "no-cache"}}
                 false)
                (reset! *channel* channel))

    ;; Default, serve the file
    {:status 200
     :headers {"Content-Type" "image/gif"}
     :body (get-file (subs (:uri req) 1))}
    ))

(defn app [req]
  (route req))

(defn stop-server []
  (when-not (nil? @server)
    ;; graceful shutdown: wait 100ms for existing requests to be finished
    ;; :timeout is optional, when no timeout, stop immediately
    (@server :timeout 100)
    (reset! server nil)
    (println "Server shut down")))

(defn start-server [& args]
  ;; The #' is useful when you want to hot-reload code
  ;; You may want to take a look: https://github.com/clojure/tools.namespace
  ;; and https://http-kit.github.io/migration.html#reload
  (reset! server (org.httpkit.server/run-server #'app {:port 12345}))
  (println "Listening for connections on http://localhost:12345"))

(defn -main []
  (start-server)
  (start-stream))

(-main)

;; Keeps bb from exiting prematurely
@(promise)
