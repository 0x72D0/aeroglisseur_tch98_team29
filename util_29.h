#ifndef UTIL29_H_INCLUDED
#define UTIL29_H_INCLUDED

/**
	\file util_29.h
	\brief Header qui defini diverse fonction utilitaire de l'equipe 29
	\author Lucas Mongrain
    \author Temuujin Darkhantsetseg
	\date 18/04/18
*/

/**
    \brief concatene deux chaine en une seule
    \param[in,out] dst chaine de destination de la concatenation
    \param[in] src1 premiere chaine a concatener
    \param[in] src2 deuxieme chaine a concatener
    \return void
*/
void string_concat(char* dst, char* src1, char* src2);

/**
    \brief ajoute a la chaine de transmission une valeur et converti un nombre en ascii par la meme occasion
    \param[in,out] str chaine de transmission
    \param[in,out] data_str tableau qui contiendra la valeur ascii du nombre envoyer
    \param[in] data valeur a ajouter a la chaine de transmission
    \return void
*/
void add_data_to_string(char* str, char* data_str, uint8_t data);

/**
    \brief initialise une chaine de caractere a une certaine valeur
    \param[in,out] dst la chaine de caractere a initialiser
    \param[in] byte la valeur a repeter sur l'ensemble de la chaine de caractere
    \param[in] num la taille de la chaine de caractere
    \return void
*/
void memory_set(char* dst, uint8_t byte, uint32_t num);
#endif
