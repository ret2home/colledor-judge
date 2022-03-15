#![allow(unused)]
use std::{thread, time};
use actix_web::http::header;
use actix_web::{get, post, web, App, HttpResponse, HttpServer, Responder};
use std::{env, fs};

pub mod api;
#[actix_web::main]
async fn main() {
    if let Err(err)=env::var("API_URL"){
        panic!("API_URL is not set");
    }
    HttpServer::new(|| {
        App::new()
        .service(api::api::judge_start_request)
        .service(api::api::judge_info)
        .service(api::api::judge_kill)
    })
    .bind(env::var("API_URL").unwrap())
    .unwrap()
    .run()
    .await;
}
